#include "request_handler.h"
#include "http_server.h"
#include <cstdio>

namespace serialization {

    json::array SerializeRoads(const model::Map::Roads& roads) {
        json::array roads_arr;
        for (const auto& road : roads) {
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

    json::array SerializeBuildings(const model::Map::Buildings& buildings) {
        json::array buildings_arr;
        for (const auto& b : buildings) {
            json::object b_obj;
            b_obj["x"] = b.GetBounds().position.x;
            b_obj["y"] = b.GetBounds().position.y;
            b_obj["w"] = b.GetBounds().size.width;
            b_obj["h"] = b.GetBounds().size.height;
            buildings_arr.push_back(b_obj);
        }
        return buildings_arr;
    }

    json::array SerializeOffices(const model::Map::Offices& offices) {
        json::array offices_arr;
        for (const auto& office : offices) {
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

    json::object SerializeMap(const model::Map& map, const extra_data::ExtraData& extra_data) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        map_obj["roads"] = SerializeRoads(map.GetRoads());
        map_obj["buildings"] = SerializeBuildings(map.GetBuildings());
        map_obj["offices"] = SerializeOffices(map.GetOffices());
        map_obj["lootTypes"] = extra_data.GetLootTypes(map);
        return map_obj;
    }

}

namespace http_handler {

std::string UrlDecode(std::string_view src) {
    std::string ret;
    int ii;
    ret.reserve(src.length());
    for (size_t i = 0; i < src.length(); i++) {
        if (src[i] == '%') {
            sscanf(src.substr(i + 1, 2).data(), "%x", &ii);
            const char ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        } else {
            ret += src[i];
        }
    }
    return ret;
}

std::string_view GetMimeType(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    if (ext == ".htm" || ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".txt") return "text/plain";
    if (ext == ".fbx") return "application/octet-stream";
    return "application/octet-stream";
}

RequestHandler::RequestHandler(model::Game& game, extra_data::ExtraData& extra_data, std::filesystem::path static_path, bool auto_tick_mode)
    : game_{game}
    , extra_data_{extra_data}
    , static_path_{std::move(static_path)}
    , auto_tick_mode_{auto_tick_mode} {
}

bool RequestHandler::IsSubPath(const std::filesystem::path& path, const std::filesystem::path& base) {
    auto rel = std::filesystem::relative(path, base);
    return !rel.empty() && rel.native()[0] != '.';
}

http::response<http::string_body> RequestHandler::MakeTextErrorResponse(
    const http::status status,
    const std::string_view message,
    const unsigned version,
    const bool keep_alive
) {
    http::response<http::string_body> response(status, version);
    response.set(http::field::content_type, "text/plain");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(keep_alive);
    response.body() = std::string(message);
    response.prepare_payload();
    return response;
}

http::response<http::string_body> RequestHandler::MakeErrorResponse(
    const http::status status,
    const std::string_view code,
    const std::string_view message,
    const unsigned version,
    const bool keep_alive
) {
    http::response<http::string_body> response(status, version);
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(keep_alive);

    json::object err_obj;
    err_obj["code"] = std::string(code);
    err_obj["message"] = std::string(message);

    response.body() = json::serialize(err_obj);
    response.prepare_payload();
    return response;
}

http::response<http::string_body> RequestHandler::MakeMapsListResponse(const unsigned version, const bool keep_alive) const {
    http::response<http::string_body> response(http::status::ok, version);
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(keep_alive);

    json::array maps_arr;
    for (const auto& map : game_.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_arr.push_back(map_obj);
    }

    response.body() = json::serialize(maps_arr);
    response.prepare_payload();
    return response;
}

http::response<http::string_body> RequestHandler::MakeMapDescriptionResponse(const model::Map& map, const unsigned version, const bool keep_alive) {
    http::response<http::string_body> response(http::status::ok, version);
    response.set(http::field::content_type, "application/json");
    response.set(http::field::cache_control, "no-cache");
    response.keep_alive(keep_alive);
    response.body() = json::serialize(serialization::SerializeMap(map, extra_data_));
    response.prepare_payload();
    return response;
}

}  // namespace http_handler