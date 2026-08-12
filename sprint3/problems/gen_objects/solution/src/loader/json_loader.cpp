#include "json_loader.h"
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace json = boost::json;
using namespace std::literals;

namespace json_loader {

namespace {

void ParseRoads(const json::value& roads_json, model::Map& map) {
    for (const auto& road_val : roads_json.as_array()) {
        const auto& road_obj = road_val.as_object();
        model::Coord x0 = road_obj.at("x0").as_int64();
        model::Coord y0 = road_obj.at("y0").as_int64();

        if (road_obj.contains("x1")) {
            model::Coord x1 = road_obj.at("x1").as_int64();
            map.AddRoad({model::Road::HORIZONTAL, {x0, y0}, x1});
        } else if (road_obj.contains("y1")) {
            model::Coord y1 = road_obj.at("y1").as_int64();
            map.AddRoad({model::Road::VERTICAL, {x0, y0}, y1});
        }
    }
}
void ParseBuildings(const json::value& buildings_json, model::Map& map) {
    for (const auto& b_val : buildings_json.as_array()) {
        const auto& b_obj = b_val.as_object();
        model::Coord x = b_obj.at("x").as_int64();
        model::Coord y = b_obj.at("y").as_int64();
        model::Dimension w = b_obj.at("w").as_int64();
        model::Dimension h = b_obj.at("h").as_int64();
        map.AddBuilding(model::Building(model::Rectangle{model::Point{x, y}, model::Size{w, h}}));
    }
}
void ParseOffices(const json::value& offices_json, model::Map& map) {
    for (const auto& o_val : offices_json.as_array()) {
        const auto& o_obj = o_val.as_object();
        std::string o_id = o_obj.at("id").as_string().c_str();
        model::Coord x = o_obj.at("x").as_int64();
        model::Coord y = o_obj.at("y").as_int64();
        model::Dimension ox = o_obj.at("offsetX").as_int64();
        model::Dimension oy = o_obj.at("offsetY").as_int64();
        map.AddOffice(model::Office(model::Office::Id{o_id}, model::Point{x, y}, model::Offset{ox, oy}));
    }
}

}  // namespace

model::Game LoadGame(const std::filesystem::path& json_path, net::io_context& ioc,
                     bool randomize_spawn_points, model::Game::LootGenerator::RandomGenerator random_generator, extra_data::ExtraData& extra_data) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_str = buffer.str();

    json::value value;
    try {
        value = json::parse(json_str);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse JSON config file: "s + e.what());
    }

    double loot_period = 5.0;
    double loot_probability = 0.5;

    if (const auto* loot_gen_config_ptr = value.as_object().if_contains("lootGeneratorConfig")) {
        const auto& loot_gen_config_obj = loot_gen_config_ptr->as_object();
        loot_period = loot_gen_config_obj.at("period").as_double();
        loot_probability = loot_gen_config_obj.at("probability").as_double();
    }

    model::Game game(ioc, randomize_spawn_points, random_generator, loot_period, loot_probability);

    if (const auto* dog_speed_ptr = value.as_object().if_contains("defaultDogSpeed")) {
        game.SetDefaultDogSpeed(dog_speed_ptr->as_double());
    }

    for (const auto& maps_array = value.as_object().at("maps").as_array();
         const auto& map_val : maps_array) {
        const auto& map_obj = map_val.as_object();

        std::string id = map_obj.at("id").as_string().c_str();
        std::string name = map_obj.at("name").as_string().c_str();

        model::Map map(model::Map::Id{id}, name);
        if (const auto* dog_speed_ptr = map_obj.if_contains("dogSpeed")) {
            map.SetDogSpeed(dog_speed_ptr->as_double());
        }

        if (const auto* loot_types_ptr = map_obj.if_contains("lootTypes")) {
            map.SetLootTypes(loot_types_ptr->as_array());
            extra_data.AddLootTypes(map, loot_types_ptr->as_array());
        }

        // region deserialisation
        if (const auto* roads_ptr = map_obj.if_contains("roads")) {
            ParseRoads(*roads_ptr, map);
        }

        if (const auto* buildings_ptr = map_obj.if_contains("buildings")) {
            ParseBuildings(*buildings_ptr, map);
        }

        if (const auto* offices_ptr = map_obj.if_contains("offices")) {
            ParseOffices(*offices_ptr, map);
        }
        // endregion

        game.AddMap(std::move(map));
    }

    return game;
}

}  // namespace json_loader