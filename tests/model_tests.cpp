#include <catch2/catch_test_macros.hpp>
#include <boost/asio/io_context.hpp>
#include "../src/model/model.h"

using namespace std::literals;

SCENARIO("Model item spawning logic") {
    namespace model = model;
    namespace net = boost::asio;

    GIVEN("A map with roads and loot type count") {
        model::Map map{model::Map::Id{"map1"s}, "Map 1"s};
        map.AddRoad({model::Road::HORIZONTAL, {0, 0}, 10});
        map.AddRoad({model::Road::VERTICAL, {0, 0}, 10});
        map.SetLootTypeCount(3);

        WHEN("GetRandomLootType is called") {
            THEN("Generated loot types are within the valid range") {
                for (int i = 0; i < 100; ++i) {
                    size_t loot_type = map.GetRandomLootType();
                    REQUIRE(loot_type < map.GetLootTypeCount());
                }
            }
        }

        WHEN("GetRandomPointOnRoads is called") {
            THEN("Generated points lie strictly on map roads") {
                for (int i = 0; i < 100; ++i) {
                    model::Point2D pos = map.GetRandomPointOnRoads();
                    REQUIRE(map.IsOnRoad(pos));
                }
            }
        }
    }

    GIVEN("A game session with a map and loot generator config") {
        net::io_context ioc;
        model::Map map{model::Map::Id{"map1"s}, "Map 1"s};
        map.AddRoad({model::Road::HORIZONTAL, {0, 0}, 10});
        map.AddRoad({model::Road::VERTICAL, {0, 0}, 10});
        map.SetLootTypeCount(2);

        std::optional<std::pair<double, double>> loot_gen_config = {{1.0, 1.0}};
        auto random_generator = [] { return 1.0; };

        model::GameSession session{&map, ioc, false, random_generator, loot_gen_config};

        WHEN("GameSession::Tick is called without any dogs") {
            THEN("No loot is generated") {
                session.Tick(1000ms, true);
                REQUIRE(session.GetLostObjects().empty());
            }
        }

        WHEN("GameSession::Tick is called with a single dog") {
            session.AddDog("Doggo"s);
            THEN("Loot is generated over time") {
                session.Tick(1000ms, true);
                REQUIRE(session.GetLostObjects().size() == 1);

                const auto& lost_object = session.GetLostObjects().begin()->second;
                REQUIRE(lost_object.type < map.GetLootTypeCount());
                REQUIRE(map.IsOnRoad(lost_object.pos));

                session.Tick(1000ms, true);
                REQUIRE(session.GetLostObjects().size() == 1);
            }
        }

        WHEN("GameSession::Tick is called with multiple dogs") {
            session.AddDog("Doggo1"s);
            session.AddDog("Doggo2"s);
            session.AddDog("Doggo3"s);
            THEN("Loot is generated up to the number of dogs") {
                session.Tick(1000ms, true);
                REQUIRE(session.GetLostObjects().size() == 3);

                for(const auto& [id, lost_object] : session.GetLostObjects()) {
                    REQUIRE(lost_object.type < map.GetLootTypeCount());
                    REQUIRE(map.IsOnRoad(lost_object.pos));
                }

                session.Tick(1000ms, true);
                REQUIRE(session.GetLostObjects().size() == 3);
            }
        }
    }
}

SCENARIO("Item value parsing and score accumulation") {
    GIVEN("A map with per-loot-type values") {
        model::Map map{model::Map::Id{"map1"s}, "Map 1"s};
        map.SetLootTypeCount(3);
        map.SetLootTypeValues({10u, 30u, 0u});

        THEN("Each loot type reports the configured value") {
            REQUIRE(map.GetLootTypeValue(0) == 10u);
            REQUIRE(map.GetLootTypeValue(1) == 30u);
            REQUIRE(map.GetLootTypeValue(2) == 0u);
        }

        THEN("Unknown loot types default to zero value") {
            REQUIRE(map.GetLootTypeValue(42) == 0u);
        }

        GIVEN("A dog with a full bag of mixed-value items") {
            model::Dog dog{model::Dog::Id{0}, "Doggo"s, model::Point2D{0.0, 0.0}, nullptr, 3};
            REQUIRE(dog.GetScore() == 0u);

            dog.CollectItem(model::BagItem{0, 0});  // value 10
            dog.CollectItem(model::BagItem{1, 1});  // value 30
            dog.CollectItem(model::BagItem{2, 2});  // value 0

            WHEN("The dog drops its items off at the base") {
                const unsigned dropped_value = dog.DropOffItems(map);

                THEN("The score increases by the total value of returned items") {
                    REQUIRE(dropped_value == 40u);
                    REQUIRE(dog.GetScore() == 40u);
                }

                THEN("The bag is emptied") {
                    REQUIRE(dog.GetBag().empty());
                }

                AND_WHEN("The dog drops off an already-empty bag") {
                    const unsigned second_drop_value = dog.DropOffItems(map);

                    THEN("The score does not change") {
                        REQUIRE(second_drop_value == 0u);
                        REQUIRE(dog.GetScore() == 40u);
                    }
                }
            }
        }
    }
}