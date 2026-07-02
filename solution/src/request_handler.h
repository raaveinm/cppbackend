#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <string_view>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace json = boost::json;

namespace api_endpoints {
constexpr std::string_view API_PREFIX = "/api/";
constexpr std::string_view MAPS_API_PREFIX = "/api/v1/maps";
} // namespace api_endpoints

class RequestHandler {
public:
    explicit RequestHandler(model::Game &game) : game_{game} {}

    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>> &&req,
                    Send &&send) {

        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            send(MakeErrorResponse(http::status::bad_request, "badRequest",
                                     "Invalid method", req.version(),
                                     req.keep_alive()));
            return;
        }

        const std::string_view target = req.target();

        if (target.starts_with(api_endpoints::MAPS_API_PREFIX)) {
            if (target == api_endpoints::MAPS_API_PREFIX) {
                send(MakeMapsListResponse(req.version(), req.keep_alive()));
                return;
            } else if (target[api_endpoints::MAPS_API_PREFIX.size()] == '/') {
                const std::string_view map_id_str =
                    target.substr(api_endpoints::MAPS_API_PREFIX.size() + 1);
                const model::Map::Id id{std::string(map_id_str)};
                const auto *map = game_.FindMap(id);

                if (!map) {
                    send(MakeErrorResponse(http::status::not_found, "mapNotFound",
                                             "Map not found", req.version(),
                                             req.keep_alive()));
                    return;
                }
                send(MakeMapDescriptionResponse(*map, req.version(),
                                                req.keep_alive()));
                return;
            }
        }

        if (target.starts_with(api_endpoints::API_PREFIX)) {
            send(MakeErrorResponse(http::status::bad_request, "badRequest",
                                     "Bad request", req.version(),
                                     req.keep_alive()));
            return;
        }

        send(MakeErrorResponse(http::status::bad_request, "badRequest",
                                 "Bad request", req.version(), req.keep_alive()));
    }

private:
    model::Game &game_;

    [[nodiscard]] static http::response<http::string_body> MakeErrorResponse(
        const http::status status, const std::string_view code,
        const std::string_view message, const unsigned version,
        const bool keep_alive) {
        http::response<http::string_body> response(status, version);
        response.set(http::field::content_type, "application/json");
        response.keep_alive(keep_alive);

        json::object err_obj;
        err_obj["code"] = code.data();
        err_obj["message"] = message.data();

        response.body() = json::serialize(err_obj);
        response.prepare_payload();
        return response;
    }

    [[nodiscard]] http::response<http::string_body>
    MakeMapsListResponse(const unsigned version, const bool keep_alive) const {
        http::response<http::string_body> response(http::status::ok, version);
        response.set(http::field::content_type, "application/json");
        response.keep_alive(keep_alive);

        json::array maps_arr;
        for (const auto &map : game_.GetMaps()) {
            json::object map_obj;
            map_obj["id"] = *map.GetId();
            map_obj["name"] = map.GetName();
            maps_arr.push_back(map_obj);
        }

        response.body() = json::serialize(maps_arr);
        response.prepare_payload();
        return response;
    }

    [[nodiscard]] static json::array GetRoadsAsJson(const model::Map &map) {
        json::array roads_arr;
        for (const auto &road : map.GetRoads()) {
            json::object road_obj;
            road_obj["x0"] = road.GetStart().x;
            road_obj["y0"] = road.GetStart().y;
            if (road.IsHorizontal()) {
                road_obj["x1"] = road.GetEnd().x;
            } else {
                road_obj["y1"] = road.GetEnd().y;
            }
            roads_arr.push_back(road_obj);
        }
        return roads_arr;
    }

    [[nodiscard]] static json::array GetBuildingsAsJson(const model::Map &map) {
        json::array buildings_arr;
        for (const auto &b : map.GetBuildings()) {
            json::object b_obj;
            b_obj["x"] = b.GetBounds().position.x;
            b_obj["y"] = b.GetBounds().position.y;
            b_obj["w"] = b.GetBounds().size.width;
            b_obj["h"] = b.GetBounds().size.height;
            buildings_arr.push_back(b_obj);
        }
        return buildings_arr;
    }

    [[nodiscard]] static json::array GetOfficesAsJson(const model::Map &map) {
        json::array offices_arr;
        for (const auto &office : map.GetOffices()) {
            json::object o_obj;
            o_obj["id"] = *office.GetId();
            o_obj["x"] = office.GetPosition().x;
            o_obj["y"] = office.GetPosition().y;
            o_obj["offsetX"] = office.GetOffset().dx;
            o_obj["offsetY"] = office.GetOffset().dy;
            offices_arr.push_back(o_obj);
        }
        return offices_arr;
    }

    [[nodiscard]] static http::response<http::string_body>
    MakeMapDescriptionResponse(const model::Map &map, const unsigned version,
                               const bool keep_alive) {
        http::response<http::string_body> response(http::status::ok, version);
        response.set(http::field::content_type, "application/json");
        response.keep_alive(keep_alive);

        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        map_obj["roads"] = GetRoadsAsJson(map);
        map_obj["buildings"] = GetBuildingsAsJson(map);
        map_obj["offices"] = GetOfficesAsJson(map);

        response.body() = json::serialize(map_obj);
        response.prepare_payload();
        return response;
    }
};

} // namespace http_handler
