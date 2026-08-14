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

#include "../util/logging.h"
#include "../model/model.h"
#include "../extra_data.h"

namespace serialization {
    namespace json = boost::json;

    json::array SerializeRoads(const model::Map::Roads& roads);
    json::array SerializeBuildings(const model::Map::Buildings& buildings);
    json::array SerializeOffices(const model::Map::Offices& offices);
    json::object SerializeMap(const model::Map& map, const extra_data::ExtraData& extra_data);
}

namespace http_handler {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;
    using namespace std::literals;

    namespace endpoints {
        using namespace std::string_view_literals;

        constexpr std::string_view API_PREFIX = "/api/"sv;
        constexpr std::string_view API_V1_PREFIX = "/api/v1/"sv;
        constexpr std::string_view MAPS = "/api/v1/maps"sv;
        constexpr std::string_view GAME_JOIN = "/api/v1/game/join"sv;
        constexpr std::string_view GAME_PLAYERS = "/api/v1/game/players"sv;
        constexpr std::string_view GAME_STATE = "/api/v1/game/state"sv;
        constexpr std::string_view GAME_PLAYER_ACTION = "/api/v1/game/player/action"sv;
        constexpr std::string_view GAME_TICK = "/api/v1/game/tick"sv;
    }

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
        explicit RequestHandler(model::Game& game, extra_data::ExtraData& extra_data, std::filesystem::path static_path, bool auto_tick_mode = false);

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            const std::string_view target = req.target();

            if (target.starts_with(endpoints::API_PREFIX)) {
                if (target == endpoints::GAME_JOIN) {
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

                if (target == endpoints::GAME_PLAYERS) {
                    if (req.method() != http::verb::get && req.method() != http::verb::head) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "GET, HEAD");
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        return send(resp);
                    }

                    return send(ExecuteAuthorized(req, [&](const model::Player& player, const model::Token&) {
                        json::object players_json;
                        if (const auto session = player.GetSession()) {
                            for (const auto& p : game_.GetPlayers()) {
                                if (p->GetSession() == session) { // <--- Filter by current session
                                    json::object player_obj;
                                    player_obj["name"] = p->GetName();
                                    players_json[std::to_string(*p->GetId())] = player_obj;
                                }
                            }
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
                        return response;
                    }));
                }

                if (target == endpoints::GAME_STATE) {
                    if (req.method() != http::verb::get && req.method() != http::verb::head) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "GET, HEAD");
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();
                            resp.body().clear();
                        }
                        return send(resp);
                    }

                    ExecuteAuthorizedAsync(req, std::forward<Send>(send), [this, &req](const model::Player& player, const model::Token&, Send&& send_async) {
                        player.GetSession()->Dispatch([this, &player, req_version = req.version(), req_keep_alive = req.keep_alive(), is_head = (req.method() == http::verb::head), send_async = std::move(send_async)]() mutable {
                            json::object root_obj;
                            if (const auto session = player.GetSession()) {
                                json::object players_json;
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
                                        json::array bag_json;
                                        for (const auto& item : p->GetDog()->GetBag()) {
                                            json::object item_obj;
                                            item_obj["id"] = item.id;
                                            item_obj["type"] = item.type;
                                            bag_json.push_back(item_obj);
                                        }
                                        player_obj["bag"] = bag_json;
                                        players_json[std::to_string(*p->GetId())] = player_obj;
                                    }
                                }
                                root_obj["players"] = players_json;

                                json::object lost_objects_json;
                                for (const auto& [id, lost_object] : session->GetLostObjects()) {
                                    json::object lost_object_obj;
                                    lost_object_obj["type"] = lost_object.type;
                                    lost_object_obj["pos"] = json::array{lost_object.pos.x, lost_object.pos.y};
                                    lost_objects_json[std::to_string(id)] = lost_object_obj;
                                }
                                root_obj["lostObjects"] = lost_objects_json;
                            }
                            http::response<http::string_body> response(http::status::ok, req_version);
                            response.set(http::field::content_type, "application/json");
                            response.set(http::field::cache_control, "no-cache");
                            response.keep_alive(req_keep_alive);

                            response.body() = json::serialize(root_obj);

                            response.prepare_payload();
                            if (is_head) {
                                response.body().clear();
                            }
                            send_async(response);
                        });
                    });
                    return;
                }

                if (target == endpoints::GAME_PLAYER_ACTION) {
                    if (req.method() != http::verb::post) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "POST");
                        return send(resp);
                    }

                    ExecuteAuthorizedAsync(req, std::forward<Send>(send), [this, &req](model::Player& player, const model::Token&, Send&& send_async) {
                        if (req.find(http::field::content_type) == req.end() || req.at(http::field::content_type) != "application/json") {
                            return send_async(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Invalid content type", req.version(), req.keep_alive()));
                        }
                        try {
                            json::value json_body = json::parse(req.body());
                            if (!json_body.is_object() || !json_body.as_object().contains("move")) {
                                return send_async(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Failed to parse action", req.version(), req.keep_alive()));
                            }

                            const auto& move_val = json_body.as_object().at("move");
                            if (!move_val.is_string()) {
                                return send_async(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Failed to parse action", req.version(), req.keep_alive()));
                            }
                            std::string move = json::value_to<std::string>(move_val);
                            double speed = player.GetSession()->GetMap()->GetDogSpeed();

                            player.GetSession()->Dispatch([&player, move, speed, req_version = req.version(), req_keep_alive = req.keep_alive(), send_async = std::move(send_async)]() mutable {
                                if (move == "L") {
                                    player.GetDog()->SetSpeed({-speed, 0});
                                    player.GetDog()->SetDirection(model::Direction::WEST);
                                } else if (move == "R") {
                                    player.GetDog()->SetSpeed({speed, 0});
                                    player.GetDog()->SetDirection(model::Direction::EAST);
                                } else if (move == "U") {
                                    player.GetDog()->SetSpeed({0, -speed});
                                    player.GetDog()->SetDirection(model::Direction::NORTH);
                                } else if (move == "D") {
                                    player.GetDog()->SetSpeed({0, speed});
                                    player.GetDog()->SetDirection(model::Direction::SOUTH);
                                } else if (move.empty()) {
                                    player.GetDog()->SetSpeed({0, 0});
                                } else {
                                    return send_async(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Failed to parse action", req_version, req_keep_alive));
                                }

                                http::response<http::string_body> response(http::status::ok, req_version);
                                response.set(http::field::content_type, "application/json");
                                response.set(http::field::cache_control, "no-cache");
                                response.keep_alive(req_keep_alive);
                                response.body() = "{}";
                                response.prepare_payload();
                                send_async(response);
                            });

                        } catch (...) {
                            return send_async(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Failed to parse action", req.version(), req.keep_alive()));
                        }
                    });
                    return;
                }

                if (target == endpoints::GAME_TICK) {
                    if (auto_tick_mode_) {
                        return send(MakeErrorResponse(
                            http::status::bad_request,
                            "badRequest",
                            "Invalid endpoint",
                            req.version(),
                            req.keep_alive()
                        ));
                    }
                    if (req.method() != http::verb::post) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Only POST method is expected", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "POST");
                        return send(resp);
                    }

                    if (req.find(http::field::content_type) == req.end() || req.at(http::field::content_type) != "application/json") {
                        return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Invalid content type", req.version(), req.keep_alive()));
                    }

                    try {
                        json::value json_body = json::parse(req.body());
                        if (!json_body.is_object() || !json_body.as_object().contains("timeDelta") || !json_body.as_object().at("timeDelta").is_int64()) {
                            return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Failed to parse tick request JSON", req.version(), req.keep_alive()));
                        }
                        
                        int64_t time_delta_ms = json_body.as_object().at("timeDelta").as_int64();
                        game_.Tick(std::chrono::milliseconds(time_delta_ms));

                        http::response<http::string_body> response(http::status::ok, req.version());
                        response.set(http::field::content_type, "application/json");
                        response.set(http::field::cache_control, "no-cache");
                        response.keep_alive(req.keep_alive());
                        response.body() = "{}";
                        response.prepare_payload();
                        return send(response);

                    } catch (...) {
                        return send(MakeErrorResponse(http::status::bad_request, "invalidArgument", "Failed to parse tick request JSON", req.version(), req.keep_alive()));
                    }
                }

                if (target.starts_with(endpoints::MAPS)) {
                    if (req.method() != http::verb::get && req.method() != http::verb::head) {
                        auto resp = MakeErrorResponse(http::status::method_not_allowed, "invalidMethod", "Invalid method", req.version(), req.keep_alive());
                        resp.set(http::field::allow, "GET, HEAD");
                        return send(resp);
                    }

                    if (target == endpoints::MAPS) {
                        auto resp = MakeMapsListResponse(req.version(), req.keep_alive());
                        if (req.method() == http::verb::head) {
                            resp.prepare_payload();                            
                            resp.body().clear();
                        }
                        send(resp);
                        return;
                    } else if (target[endpoints::MAPS.size()] == '/') {
                        const std::string_view map_id_str = target.substr(endpoints::MAPS.size() + 1);
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
        extra_data::ExtraData& extra_data_;
        std::filesystem::path static_path_;
        bool auto_tick_mode_ = false;

        template <typename Body, typename Allocator, typename Fn>
        auto ExecuteAuthorized(const http::request<Body, http::basic_fields<Allocator>>& req, Fn&& action) {
            if (auto token = TryExtractToken(req)) {
                if (auto* player = game_.FindPlayerByToken(*token); player) {
                    return action(*player, *token);
                }
                return MakeErrorResponse(
                    http::status::unauthorized, 
                    "unknownToken", 
                    "Player token has not been found", 
                    req.version(), 
                    req.keep_alive()
                );
            }
            return MakeErrorResponse(
                http::status::unauthorized, 
                "invalidToken", 
                "Authorization header is required", 
                req.version(), 
                req.keep_alive()
            );
        }

        template <typename Body, typename Allocator, typename Send, typename Fn>
        void ExecuteAuthorizedAsync(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send, Fn&& action) {
            if (auto token = TryExtractToken(req)) {
                if (auto* player = game_.FindPlayerByToken(*token); player) {
                    return action(*player, *token, std::forward<Send>(send));
                }
                return send(MakeErrorResponse(
                    http::status::unauthorized,
                    "unknownToken",
                    "Player token has not been found",
                    req.version(),
                    req.keep_alive()
                ));
            }
            return send(MakeErrorResponse(
                http::status::unauthorized,
                "invalidToken",
                "Authorization header is required",
                req.version(),
                req.keep_alive()
            ));
        }

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

        [[nodiscard]] http::response<http::string_body> MakeMapDescriptionResponse(const model::Map& map, unsigned version, bool keep_alive);
    };
} // namespace http_handler
