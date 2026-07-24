#pragma once
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <chrono>
#include <filesystem>
#include <utility>
#include <string>
#include <string_view>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <optional>

#include "logging.h"
#include "model.h"

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    using namespace std::literals;

    std::string UrlDecode(std::string_view src);
    std::string_view GetMimeType(const std::filesystem::path& path);

    class RequestHandler; // Forward declaration

    class LoggingRequestHandler {
    public:
        explicit LoggingRequestHandler(RequestHandler& handler) : handler_{handler} {}

        LoggingRequestHandler(const LoggingRequestHandler&) = delete;
        LoggingRequestHandler& operator=(const LoggingRequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, tcp::endpoint endpoint, Send&& send) {
            log_request(req, endpoint);

            auto start_time = std::chrono::steady_clock::now();

            handler_(std::move(req), [send = std::forward<Send>(send), start_time, endpoint, this](auto&& response) {
                log_response(response, endpoint, start_time);
                send(std::forward<decltype(response)>(response));
            });
        }

    private:
        RequestHandler& handler_;

        template <typename Body, typename Allocator>
        void log_request(const http::request<Body, http::basic_fields<Allocator>>& req, const tcp::endpoint& endpoint) {
            json::object data;
            data["ip"] = endpoint.address().to_string();
            data["URI"] = std::string(req.target());
            data["method"] = std::string(req.method_string());
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value("AdditionalData", json::value(data)) << "request received"sv;
        }

        template <typename Response>
        void log_response(const Response& resp, const tcp::endpoint& endpoint, std::chrono::steady_clock::time_point start_time) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            json::object data;
            data["ip"] = endpoint.address().to_string();
            data["response_time"] = duration.count();
            data["code"] = resp.result_int();

            auto content_type_it = resp.find(http::field::content_type);
            if (content_type_it != resp.end()) {
                data["content_type"] = std::string(content_type_it->value());
            } else {
                data["content_type"] = "null";
            }
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value("AdditionalData", json::value(data)) << "response sent"sv;
        }
    };

    class RequestHandler {
    public:
        explicit RequestHandler(model::Game& game, std::filesystem::path static_path);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            const std::string_view target = req.target();

            if (target.starts_with("/api/")) {
                if (target == "/api/v1/game/join"sv) {
                    if (req.method() != http::verb::post) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Only POST method is expected", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "POST");
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        return send(resp);
                    }

                    try {
                        json::parse_options opt;
                        opt.allow_invalid_utf8 = true;

                        boost::system::error_code ec;
                        json::value json_body = json::parse(req.body(), ec, {}, opt);

                        if (ec || !json_body.is_object()) {
                            return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Join game request parse error", req.version(), req.keep_alive()));
                        }

                        const auto& obj = json_body.as_object();
                        if (!obj.contains("userName") || !obj.contains("mapId") ||
                            !obj.at("userName").is_string() || !obj.at("mapId").is_string()) {
                            return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Join game request parse error", req.version(), req.keep_alive()));
                        }

                        const std::string user_name = json::value_to<std::string>(obj.at("userName"));
                        const std::string map_id = json::value_to<std::string>(obj.at("mapId"));

                        if (user_name.empty()) {
                            return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Invalid name", req.version(), req.keep_alive()));
                        }

                        model::Map::Id m_id{map_id};
                        if (!game_.FindMap(m_id)) {
                            return send(MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found", req.version(), req.keep_alive()));
                        }

                        auto [player, token] = game_.AddPlayer(m_id, user_name);

                        http::response<http::string_body> response(http::status::ok, req.version());
                        response.set(http::field::content_type, "application/json");
                        response.set(http::field::cache_control, "no-cache");
                        response.keep_alive(req.keep_alive());

                        json::object resp_obj;
                        resp_obj["authToken"] = *token;
                        resp_obj["playerId"] = *player.GetId();

                        response.body() = json::serialize(resp_obj);
                        response.prepare_payload();
                        return send(response);

                    } catch (...) {
                        return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Join game request parse error", req.version(), req.keep_alive()));
                    }
                }

                if (target == "/api/v1/game/players"sv) {
                    if (req.method() != http::verb::get && req.method() != http::verb::head) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "GET, HEAD");
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        return send(resp);
                    }

                    
                    if (auto token = TryExtractToken(req)) {
                        const model::Player* player = game_.FindPlayerByToken(*token);
                        if (!player) {
                            return send(MakeErrorResponse(http::status::unauthorized, "unknownToken", "Player token has not been found", req.version(), req.keep_alive()));
                        }
                    } else {
                        return send(MakeErrorResponse(http::status::unauthorized, "invalidToken", "Authorization header is missing or invalid", req.version(), req.keep_alive()));
                    }

                    json::object players_json;
                    for (const auto& p : game_.GetPlayers()) {
                        json::object player_obj;
                        player_obj["name"] = p->GetName();
                        players_json[std::to_string(*p->GetId())] = player_obj;
                    }
                    
                    http::response<http::string_body> response(http::status::ok, req.version());
                    response.set(http::field::content_type, "application/json");
                    response.set(http::field::cache_control, "no-cache");
                    response.keep_alive(req.keep_alive());

                    response.body() = json::serialize(players_json);
                    response.prepare_payload();
                    if (req.method() == http::verb::head) {
                        response.body().clear();
                    }
                    return send(response);
                }

                if (target == "/api/v1/game/state"sv) {
                    if (req.method() != http::verb::get && req.method() != http::verb::head) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "GET, HEAD");
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        return send(resp);
                    }

                    auto token = TryExtractToken(req);
                    if (!token) {
                        return send(MakeErrorResponse(http::status::unauthorized, "invalidToken", "Authorization header is missing", req.version(), req.keep_alive()));
                    }

                    const model::Player* player = game_.FindPlayerByToken(*token);
                    if (!player) {
                        return send(MakeErrorResponse(http::status::unauthorized, "unknownToken", "Player token has not been found", req.version(), req.keep_alive()));
                    }

                    json::object players_json;
                    
                    if (const auto session = player->GetSession()) {
                        for (const auto& p : game_.GetPlayers()) {
                            if (p->GetSession() == session) {
                                json::object player_obj;
                                player_obj["pos"] = json::array{p->GetDog()->GetPosition().x, p->GetDog()->GetPosition().y};
                                player_obj["speed"] = json::array{p->GetDog()->GetSpeed().x, p->GetDog()->GetSpeed().y};

                                switch (p->GetDog()->GetDirection()) {
                                    case model::Direction::NORTH:
                                        player_obj["dir"] = "U";
                                        break;
                                    case model::Direction::SOUTH:
                                        player_obj["dir"] = "D";
                                        break;
                                    case model::Direction::WEST:
                                        player_obj["dir"] = "L";
                                        break;
                                    case model::Direction::EAST:
                                        player_obj["dir"] = "R";
                                        break;
                                }
                                players_json[std::to_string(*p->GetId())] = player_obj;
                            }
                        }
                    }

                    http::response<http::string_body> response(http::status::ok, req.version());
                    response.set(http::field::content_type, "application/json");
                    response.set(http::field::cache_control, "no-cache");
                    response.keep_alive(req.keep_alive());
					
                    json::object root_obj;
                    root_obj["players"] = players_json;
                    response.body() = json::serialize(root_obj);
                    
                    response.prepare_payload();
                    if (req.method() == http::verb::head) {
                        response.body().clear();
                    }
                    return send(response);
                }

                if (req.method() != http::verb::get && req.method() != http::verb::head) {
                    return send(MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive()));
                }

                if (constexpr std::string_view api_prefix = "/api/v1/maps"; target.starts_with(api_prefix)) {
                    if (target == api_prefix) {
                        auto resp = MakeMapsListResponse(req.version(), req.keep_alive());
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        send(resp);
                        return;
                    } else if (target[api_prefix.size()] == '/') {
                        const std::string_view map_id_str = target.substr(api_prefix.size() + 1);
                        const model::Map::Id id{std::string(map_id_str)};
                        const auto* map = game_.FindMap(id);

                        if (!map) {
                            return send(MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found", req.version(), req.keep_alive()));
                        }
                        auto resp = MakeMapDescriptionResponse(*map, req.version(), req.keep_alive());
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        send(resp);
                        return;
                    }
                }
                return send(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request", req.version(), req.keep_alive()));
            }

            try {
                std::string decoded_path = UrlDecode(target);
                if (decoded_path.back() == '/') {
                    decoded_path += "index.html";
                }

                const std::filesystem::path file_path = static_path_ / decoded_path.substr(1);

                if (!IsSubPath(file_path, static_path_)) {
                    return send(MakeTextErrorResponse(http::status::bad_request, "Bad Request: Path out of root", req.version(), req.keep_alive()));
                }

                if (!std::filesystem::exists(file_path)) {
                    return send(MakeTextErrorResponse(http::status::not_found, "File Not Found", req.version(), req.keep_alive()));
                }

                http::response<http::file_body> res;
                res.version(req.version());
                res.result(http::status::ok);
                res.set(http::field::content_type, GetMimeType(file_path));

                http::file_body::value_type file;
                if (beast::error_code ec; file.open(file_path.c_str(), beast::file_mode::read, ec), ec) {
                    return send(MakeErrorResponse(http::status::internal_server_error, "serverError", "Failed to open file", req.version(), req.keep_alive()));
                }
                res.body() = std::move(file);

                res.prepare_payload();
                send(std::move(res));
            } catch (const std::exception& e) {
                send(MakeErrorResponse(http::status::internal_server_error, "serverError", e.what(), req.version(), req.keep_alive()));
            }
        }

private:
model::Game& game_;
std::filesystem::path static_path_;

template <typename Body, typename Allocator>
std::optional<model::Token> TryExtractToken(const http::request<Body, http::basic_fields<Allocator>>& req) {
    auto auth_header = req.find(http::field::authorization);
    if (auth_header == req.end()) {
        return std::nullopt;
    }

    std::string auth_token = std::string(auth_header->value());
    constexpr std::string_view bearer_prefix = "Bearer ";
    if (auth_token.rfind(bearer_prefix, 0) != 0 || auth_token.size() < bearer_prefix.size() + 32) {
        return std::nullopt;
    }

    std::string token_str = auth_token.substr(bearer_prefix.size());
    if (token_str.size() != 32) {
        return std::nullopt;
    }

    return model::Token{token_str};
}

static bool IsSubPath(const std::filesystem::path& path, const std::filesystem::path& base);
        [[nodiscard]] static http::response<http::string_body> MakeErrorResponse(
            http::status status,
            std::string_view code,
            std::string_view message,
            unsigned version,
            bool keep_alive
        );

        [[nodiscard]] static http::response<http::string_body> MakeTextErrorResponse(
            http::status status,
            std::string_view message,
            unsigned version,
            bool keep_alive
        );

        [[nodiscard]] http::response<http::string_body> MakeMapsListResponse(unsigned version, bool keep_alive) const;

        [[nodiscard]] static http::response<http::string_body> MakeMapDescriptionResponse(const model::Map& map, unsigned version, bool keep_alive);
    };
} // namespace http_handler