#pragma once
#include <filesystem>
#include <boost/beast/core/file.hpp>

#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <string_view>

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
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
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

}  // namespace http_handler