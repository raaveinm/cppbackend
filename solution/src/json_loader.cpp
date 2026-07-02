#include "json_loader.h"
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace json = boost::json;

namespace json_loader {

namespace {

void LoadRoads(model::Map &map, const json::object &map_obj) {
    if (map_obj.contains("roads")) {
        for (const auto &road_val : map_obj.at("roads").as_array()) {
            const auto &road_obj = road_val.as_object();
            model::Coord x0 = road_obj.at("x0").as_int64();
            model::Coord y0 = road_obj.at("y0").as_int64();

            if (road_obj.contains("x1")) {
                model::Coord x1 = road_obj.at("x1").as_int64();
                map.AddRoad(
                    model::Road(model::Road::HORIZONTAL, model::Point{x0, y0},
                                x1));
            } else if (road_obj.contains("y1")) {
                model::Coord y1 = road_obj.at("y1").as_int64();
                map.AddRoad(
                    model::Road(model::Road::VERTICAL, model::Point{x0, y0}, y1));
            }
        }
    }
}

void LoadBuildings(model::Map &map, const json::object &map_obj) {
    if (map_obj.contains("buildings")) {
        for (const auto &b_val : map_obj.at("buildings").as_array()) {
            const auto &b_obj = b_val.as_object();
            model::Coord x = b_obj.at("x").as_int64();
            model::Coord y = b_obj.at("y").as_int64();
            model::Dimension w = b_obj.at("w").as_int64();
            model::Dimension h = b_obj.at("h").as_int64();
            map.AddBuilding(model::Building(
                model::Rectangle{model::Point{x, y}, model::Size{w, h}}));
        }
    }
}

void LoadOffices(model::Map &map, const json::object &map_obj) {
    if (map_obj.contains("offices")) {
        for (const auto &o_val : map_obj.at("offices").as_array()) {
            const auto &o_obj = o_val.as_object();
            std::string o_id = o_obj.at("id").as_string().c_str();
            model::Coord x = o_obj.at("x").as_int64();
            model::Coord y = o_obj.at("y").as_int64();
            model::Dimension ox = o_obj.at("offsetX").as_int64();
            model::Dimension oy = o_obj.at("offsetY").as_int64();
            map.AddOffice(model::Office(model::Office::Id{o_id},
                                        model::Point{x, y},
                                        model::Offset{ox, oy}));
        }
    }
}

} // namespace

model::Game LoadGame(const std::filesystem::path &json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " +
                                 json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_str = buffer.str();
    json::value value;
    try {
        value = json::parse(json_str);
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse config file: " +
                                 std::string(e.what()));
    }

    model::Game game;

    for (const auto &maps_array = value.as_object().at("maps").as_array();
         const auto &map_val : maps_array) {
        const auto &map_obj = map_val.as_object();

        std::string id = map_obj.at("id").as_string().c_str();
        std::string name = map_obj.at("name").as_string().c_str();

        model::Map map(model::Map::Id{id}, name);

        LoadRoads(map, map_obj);
        LoadBuildings(map, map_obj);
        LoadOffices(map, map_obj);

        game.AddMap(std::move(map));
    }

    return game;
}

} // namespace json_loader
