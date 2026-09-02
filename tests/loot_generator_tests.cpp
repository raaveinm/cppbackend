#include "../src/loot_generator.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace loot_gen;
using namespace std::literals;

SCENARIO("LootGenerator") {
    GIVEN("a LootGenerator with base interval 1s and probability 1.0") {
        LootGenerator::TimeInterval base_interval = 1s;
        double probability = 1.0;
        LootGenerator generator{base_interval, probability};

        WHEN("item count is greater than or equal to player count") {
            THEN("Generate returns 0") {
                // Item count equals player count
                CHECK(generator.Generate(1s, 10, 10) == 0);
                // Item count exceeds player count
                CHECK(generator.Generate(1s, 20, 10) == 0);
            }
        }

        WHEN("player count is greater than item count") {
            THEN("Generate returns a number of items based on the time interval") {
                // Time interval equals base period
                CHECK(generator.Generate(1s, 5, 10) == 5);
                // Time interval is smaller than base period
                CHECK(generator.Generate(500ms, 5, 10) == 5);
                // Time interval is larger than base period
                CHECK(generator.Generate(2s, 5, 10) == 5);
            }
        }
    }

    GIVEN("a LootGenerator with base interval 1s and probability 0.5") {
        LootGenerator::TimeInterval base_interval = 1s;
        double probability = 0.5;
        LootGenerator generator{base_interval, probability};

        WHEN("the time interval is smaller than the base period") {
            THEN("the number of generated items is reduced") {
                CHECK(generator.Generate(250ms, 0, 10) == 2);
                CHECK(generator.Generate(500ms, 0, 10) == 3);
            }
        }

        WHEN("the time interval is larger than the base period") {
            THEN("the number of generated items increases") {
                CHECK(generator.Generate(2s, 0, 10) == 8);
                CHECK(generator.Generate(3s, 0, 10) == 9);
            }
        }
    }

    GIVEN("a LootGenerator with a custom random number generator") {
        LootGenerator::TimeInterval base_interval = 1s;
        double probability = 0.5;
        auto custom_random = [] {
            static int call_count = 0;
            if (call_count++ % 2 == 0) {
                return 0.2;
            }
            return 0.8;
        };
        LootGenerator generator{base_interval, probability, custom_random};

        WHEN("Generate is called multiple times") {
            THEN("the number of generated items reflects the custom random values") {
                CHECK(generator.Generate(1s, 0, 10) == 1);
                CHECK(generator.Generate(1s, 0, 10) == 4);
            }
        }
    }
}