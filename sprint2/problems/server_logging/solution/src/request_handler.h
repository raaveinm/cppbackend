#pragma once
#include <filesystem>
#include <boost/beast/core/file.hpp>
#include <chrono>

#include "http_server.h"
#include "model.h"
#include "logging.h"

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

std::string UrlDecode(std::string_view src);
std::string_view GetMimeType(const std::filesystem::path& path);

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, std::filesystem::path static_path);

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& remote_ip) {
        const auto send_error = [&req, &send](http::status status, std::string_view code, std::string_view message) {
            send(MakeErrorResponse(status, code, message, req.version(), req.keep_alive()));
        };

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return send_error(http::status::method_not_allowed, "invalidMethod", "Invalid method");
        }

        const std::string_view target = req.target();

        if (target.starts_with("/api/")) {
            if (constexpr std::string_view api_prefix = "/api/v1/maps"; target.starts_with(api_prefix)) {
                if (target == api_prefix) {
                    send(MakeMapsListResponse(req.version(), req.keep_alive()));
                    return;
                } else if (target[api_prefix.size()] == '/') {
                    const std::string_view map_id_str = target.substr(api_prefix.size() + 1);
                    const model::Map::Id id{std::string(map_id_str)};
                    const auto* map = game_.FindMap(id);

                    if (!map) {
                        return send_error(http::status::not_found, "mapNotFound", "Map not found");
                    }
                    send(MakeMapDescriptionResponse(*map, req.version(), req.keep_alive()));
                    return;
                }
            }
            return send_error(http::status::bad_request, "badRequest", "Bad request");
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
                 return send_error(http::status::internal_server_error, "serverError", "Failed to open file");
            }
            res.body() = std::move(file);

            res.prepare_payload();
            send(std::move(res));
        } catch (const std::exception& e) {
            send_error(http::status::internal_server_error, "serverError", e.what());
        }
    }

private:
    model::Game& game_;
    std::filesystem::path static_path_;

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

template <typename RequestHandler>
class LoggingRequestHandler {
public:
    LoggingRequestHandler(RequestHandler& decorated) : decorated_(decorated) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, const std::string& remote_ip) {
        auto start_time = std::chrono::high_resolution_clock::now();

        json::value req_data = {
            {"ip", remote_ip},
            {"URI", std::string(req.target())},
            {"method", std::string(req.method_string())}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, req_data) << "request received";

        auto logging_send = [send, start_time, remote_ip](auto&& response) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

            json::value resp_data;
            resp_data.get_object()["response_time"] = duration.count();
            resp_data.get_object()["code"] = response.result_int();
            if(response.has_content_length()) {
                resp_data.get_object()["content_type"] = std::string(response[http::field::content_type]);
            } else {
                resp_data.get_object()["content_type"] = nullptr;
            }

            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, resp_data) << "response sent";
            send(std::forward<decltype(response)>(response));
        };
        decorated_(std::move(req), logging_send, remote_ip);
    }

private:
    RequestHandler& decorated_;
};

}
