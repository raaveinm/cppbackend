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

SCENARIO("Dog bag capacity") {
    namespace net = boost::asio;
    net::io_context ioc;
    model::Map map{model::Map::Id{"bag_map"s}, "Bag Map"s};
    map.AddRoad({model::Road::HORIZONTAL, {0, 0}, 5});
    map.SetBagCapacity(2);

    auto random_generator = [] { return 0.0; };
    model::GameSession session{&map, ioc, false, random_generator, std::nullopt};

    GIVEN("A freshly spawned dog") {
        auto* dog = session.AddDog("Rex"s);

        THEN("Its bag starts empty and respects the configured capacity") {
            REQUIRE(dog->GetBag().empty());
            REQUIRE_FALSE(dog->IsBagFull());

            REQUIRE(dog->CollectItem({1, 0}));
            REQUIRE(dog->CollectItem({2, 1}));
            REQUIRE(dog->IsBagFull());
            REQUIRE_FALSE(dog->CollectItem({3, 0}));
            REQUIRE(dog->GetBag().size() == 2);

            dog->EmptyBag();
            REQUIRE(dog->GetBag().empty());
            REQUIRE_FALSE(dog->IsBagFull());
        }
    }
}

SCENARIO("Item collection and base return during a tick") {
    namespace net = boost::asio;
    net::io_context ioc;

    GIVEN("A session with limited bag capacity and two lost objects on the same road") {
        model::Map map{model::Map::Id{"capacity_map"s}, "Capacity Map"s};
        map.AddRoad({model::Road::HORIZONTAL, {0, 0}, 20});
        map.SetLootTypeCount(1);
        map.SetBagCapacity(1);

        std::optional<std::pair<double, double>> loot_gen_config = {{1.0, 1.0}};
        auto random_generator = [] { return 1.0; };
        model::GameSession session{&map, ioc, false, random_generator, loot_gen_config};

        auto* stationary_dog = session.AddDog("Stationary"s);
        auto* mover = session.AddDog("Mover"s);
        (void)stationary_dog;

        session.Tick(1000ms, true);
        REQUIRE(session.GetLostObjects().size() == 2);
        REQUIRE(mover->GetBag().empty());

        WHEN("The mover crosses the whole road in a single tick") {
            mover->SetPosition({0.0, 0.0});
            mover->SetSpeed({20.0, 0.0});
            session.Tick(1000ms, true);

            THEN("Only as many items as fit in the bag are collected") {
                REQUIRE(mover->GetBag().size() == 1);
                REQUIRE(session.GetLostObjects().size() == 1);
            }
        }
    }

    GIVEN("A session with a base at the far end of the road") {
        model::Map map{model::Map::Id{"return_map"s}, "Return Map"s};
        map.AddRoad({model::Road::HORIZONTAL, {0, 0}, 20});
        map.SetLootTypeCount(1);
        map.AddOffice(model::Office(model::Office::Id{"base1"s}, model::Point{20, 0}, model::Offset{0, 0}));

        std::optional<std::pair<double, double>> loot_gen_config = {{1.0, 1.0}};
        auto random_generator = [] { return 1.0; };
        model::GameSession session{&map, ioc, false, random_generator, loot_gen_config};

        auto* dog = session.AddDog("Courier"s);
        session.Tick(1000ms, true);
        REQUIRE(session.GetLostObjects().size() == 1);

        WHEN("The dog travels the whole road to the base in one tick") {
            dog->SetPosition({0.0, 0.0});
            dog->SetSpeed({20.0, 0.0});
            session.Tick(1000ms, true);

            THEN("It picks up the item along the way and deposits it at the base") {
                REQUIRE(session.GetLostObjects().empty());
                REQUIRE(dog->GetBag().empty());
            }
        }
    }

    GIVEN("A session with a stationary dog placed exactly on an item") {
        model::Map map{model::Map::Id{"stationary_map"s}, "Stationary Map"s};
        map.AddRoad({model::Road::HORIZONTAL, {0, 0}, 20});
        map.SetLootTypeCount(1);

        std::optional<std::pair<double, double>> loot_gen_config = {{1.0, 1.0}};
        auto random_generator = [] { return 1.0; };
        model::GameSession session{&map, ioc, false, random_generator, loot_gen_config};

        auto* dog = session.AddDog("Idle"s);
        session.Tick(1000ms, true);
        REQUIRE(session.GetLostObjects().size() == 1);
        const auto item_pos = session.GetLostObjects().begin()->second.pos;

        WHEN("The dog does not move even though it sits on the item") {
            dog->SetPosition(item_pos);
            session.Tick(1000ms, true);

            THEN("The item is not collected") {
                REQUIRE(session.GetLostObjects().size() == 1);
                REQUIRE(dog->GetBag().empty());
            }
        }
    }
}