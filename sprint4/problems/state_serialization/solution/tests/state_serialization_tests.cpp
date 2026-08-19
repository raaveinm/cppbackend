#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/io_context.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "../src/model/model.h"
#include "../src/serialization/model_serialization.h"
#include "../src/serialization/state_serialization.h"

using namespace model;
using namespace std::literals;

namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

Map MakeMap() {
    Map map{Map::Id{"map1"s}, "Map 1"s};
    map.AddRoad({Road::HORIZONTAL, {0, 0}, 10});
    map.SetLootTypeCount(2);
    map.SetLootTypeValues({10u, 20u});
    map.SetDefaultBagCapacity(3);
    return map;
}

}  // namespace

SCENARIO_METHOD(Fixture, "Point2D serialization") {
    GIVEN("a point") {
        const Point2D p{10.5, -20.25};
        WHEN("the point is serialized") {
            output_archive << p;

            THEN("it is equal to the point after deserialization") {
                InputArchive input_archive{strm};
                Point2D restored{};
                input_archive >> restored;
                CHECK(p.x == restored.x);
                CHECK(p.y == restored.y);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "DogRepr serialization") {
    GIVEN("a dog with speed, direction, score and bag contents") {
        Dog dog{Dog::Id{42}, "Pluto"s, Point2D{1.5, 2.5}, nullptr, 3};
        dog.SetSpeed({2.3, -1.2});
        dog.SetDirection(Direction::EAST);
        dog.SetScore(17u);
        REQUIRE(dog.CollectItem(BagItem{10, 2u}));

        WHEN("the dog is wrapped in a DogRepr and serialized") {
            {
                state_serialization::DogRepr repr{dog};
                output_archive << repr;
            }

            THEN("it can be deserialized back into an equivalent DogRepr") {
                InputArchive input_archive{strm};
                state_serialization::DogRepr restored;
                input_archive >> restored;

                CHECK(restored.GetId() == dog.GetId());
                CHECK(restored.GetName() == dog.GetName());
                CHECK(restored.GetPosition().x == dog.GetPosition().x);
                CHECK(restored.GetPosition().y == dog.GetPosition().y);
                CHECK(restored.GetSpeed().x == dog.GetSpeed().x);
                CHECK(restored.GetSpeed().y == dog.GetSpeed().y);
                CHECK(restored.GetDirection() == dog.GetDirection());
                CHECK(restored.GetScore() == dog.GetScore());
                CHECK(restored.GetBagCapacity() == dog.GetBagCapacity());
                REQUIRE(restored.GetBag().size() == dog.GetBag().size());
                CHECK(restored.GetBag()[0].id == dog.GetBag()[0].id);
                CHECK(restored.GetBag()[0].type == dog.GetBag()[0].type);
            }
        }
    }
}

SCENARIO("Full game state round trip") {
    GIVEN("a game with one map, a player, and a lost object on the ground") {
        boost::asio::io_context ioc;
        auto random_generator = [] { return 0.5; };
        Game game{ioc, false, random_generator};
        game.AddMap(MakeMap());

        auto [player, token] = game.AddPlayer(Map::Id{"map1"s}, "Alice"s);
        player.GetDog()->SetPosition({3.0, 4.0});
        player.GetDog()->SetSpeed({1.0, 0.0});
        player.GetDog()->SetDirection(Direction::WEST);
        player.GetDog()->SetScore(5u);
        REQUIRE(player.GetDog()->CollectItem(BagItem{0, 1}));

        auto* session = game.FindSession(Map::Id{"map1"s});
        REQUIRE(session != nullptr);
        session->RestoreLostObject(7u, LostObject{7u, 1u, Point2D{6.0, 6.0}});

        WHEN("the game is serialized and restored into a fresh game with the same maps") {
            std::stringstream strm;
            {
                OutputArchive output_archive{strm};
                state_serialization::GameStateRepr repr{game};
                output_archive << repr;
            }

            Game restored_game{ioc, false, random_generator};
            restored_game.AddMap(MakeMap());
            {
                InputArchive input_archive{strm};
                state_serialization::GameStateRepr repr;
                input_archive >> repr;
                repr.Restore(restored_game);
            }

            THEN("the restored game has the same dog and lost object state") {
                auto* restored_session = restored_game.FindSession(Map::Id{"map1"s});
                REQUIRE(restored_session != nullptr);
                REQUIRE(restored_session->GetDogs().size() == 1);

                const auto& restored_dog = *restored_session->GetDogs().front();
                CHECK(restored_dog.GetId() == player.GetDog()->GetId());
                CHECK(restored_dog.GetName() == player.GetDog()->GetName());
                CHECK(restored_dog.GetPosition().x == player.GetDog()->GetPosition().x);
                CHECK(restored_dog.GetPosition().y == player.GetDog()->GetPosition().y);
                CHECK(restored_dog.GetScore() == player.GetDog()->GetScore());
                REQUIRE(restored_dog.GetBag().size() == 1);
                CHECK(restored_dog.GetBag()[0].type == 1);

                REQUIRE(restored_session->GetLostObjects().size() == 1);
                CHECK(restored_session->GetLostObjects().at(7u).type == 1u);
            }

            THEN("the player's token still resolves to the same dog") {
                auto* restored_player = restored_game.FindPlayerByToken(token);
                REQUIRE(restored_player != nullptr);
                CHECK(restored_player->GetId() == player.GetId());
                CHECK(restored_player->GetDog()->GetId() == player.GetDog()->GetId());
            }

            THEN("new players joining after restore don't collide with restored ids") {
                auto [new_player, new_token] = restored_game.AddPlayer(Map::Id{"map1"s}, "Bob"s);
                CHECK(new_player.GetId() != player.GetId());
                CHECK(new_player.GetDog()->GetId() != player.GetDog()->GetId());
                CHECK(new_token != token);
            }
        }

        WHEN("the saved state references a map that no longer exists") {
            std::stringstream strm;
            {
                OutputArchive output_archive{strm};
                state_serialization::GameStateRepr repr{game};
                output_archive << repr;
            }

            Game empty_game{ioc, false, random_generator};
            InputArchive input_archive{strm};
            state_serialization::GameStateRepr repr;
            input_archive >> repr;

            THEN("restoring throws instead of silently dropping data") {
                CHECK_THROWS_AS(repr.Restore(empty_game), std::runtime_error);
            }
        }
    }
}

SCENARIO("SaveGameState and LoadGameState round trip through the filesystem") {
    GIVEN("a game with a player, and a target state file path") {
        boost::asio::io_context ioc;
        auto random_generator = [] { return 0.5; };
        Game game{ioc, false, random_generator};
        game.AddMap(MakeMap());
        auto [player, token] = game.AddPlayer(Map::Id{"map1"s}, "Alice"s);
        player.GetDog()->SetScore(9u);

        const auto dir = std::filesystem::temp_directory_path() / "state_serialization_tests";
        std::filesystem::create_directories(dir);
        const auto path = dir / "state.txt";
        std::filesystem::remove(path);
        const auto tmp_path = std::filesystem::path(path.string() + ".tmp");

        WHEN("the state is saved") {
            state_serialization::SaveGameState(game, path);

            THEN("the destination file exists and no temp file is left behind") {
                CHECK(std::filesystem::exists(path));
                CHECK_FALSE(std::filesystem::exists(tmp_path));
            }

            THEN("loading it into a fresh game reproduces the player's dog") {
                Game restored_game{ioc, false, random_generator};
                restored_game.AddMap(MakeMap());
                state_serialization::LoadGameState(restored_game, path);

                auto* restored_player = restored_game.FindPlayerByToken(token);
                REQUIRE(restored_player != nullptr);
                CHECK(restored_player->GetDog()->GetScore() == 9u);
            }
        }

        WHEN("the state file is corrupted") {
            {
                std::ofstream out(path);
                out << "not a valid archive";
            }

            THEN("loading it throws instead of silently starting from scratch") {
                Game fresh_game{ioc, false, random_generator};
                fresh_game.AddMap(MakeMap());
                CHECK_THROWS(state_serialization::LoadGameState(fresh_game, path));
            }
        }

        std::filesystem::remove_all(dir);
    }
}
