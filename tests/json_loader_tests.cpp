#include <catch2/catch_test_macros.hpp>
#include <boost/asio/io_context.hpp>
#include <filesystem>
#include <fstream>

#include "../src/loader/json_loader.h"
#include "../src/extra_data.h"

using namespace std::literals;

namespace {

constexpr std::string_view CONFIG_JSON = R"({
  "defaultDogSpeed": 3.0,
  "defaultBagCapacity": 3,
  "lootGeneratorConfig": { "period": 5.0, "probability": 0.5 },
  "maps": [
    {
      "id": "map1",
      "name": "Map 1",
      "roads": [ { "x0": 0, "y0": 0, "x1": 10 } ],
      "offices": [ { "id": "o1", "x": 0, "y": 0, "offsetX": 0, "offsetY": 0 } ],
      "lootTypes": [
        { "name": "key", "value": 10 },
        { "name": "wallet", "value": 30 },
        { "name": "trinket" }
      ]
    }
  ]
})";

}  // namespace

SCENARIO("Loot type values are parsed from the config file") {
    GIVEN("A config file with per-loot-type \"value\" fields") {
        const auto tmp_path = std::filesystem::temp_directory_path() / "scores_test_config.json";
        {
            std::ofstream out(tmp_path);
            out << CONFIG_JSON;
        }

        boost::asio::io_context ioc;
        extra_data::ExtraData extra_data;
        auto random_generator = [] { return 0.0; };

        WHEN("The game is loaded from the config") {
            model::Game game = json_loader::LoadGame(tmp_path, ioc, false, random_generator, extra_data);

            THEN("The map exposes the parsed values for every loot type") {
                const auto* map = game.FindMap(model::Map::Id{"map1"s});
                REQUIRE(map != nullptr);
                REQUIRE(map->GetLootTypeCount() == 3);
                REQUIRE(map->GetLootTypeValue(0) == 10u);
                REQUIRE(map->GetLootTypeValue(1) == 30u);
            }

            THEN("A loot type without a \"value\" field defaults to zero") {
                const auto* map = game.FindMap(model::Map::Id{"map1"s});
                REQUIRE(map != nullptr);
                REQUIRE(map->GetLootTypeValue(2) == 0u);
            }
        }

        std::filesystem::remove(tmp_path);
    }
}
