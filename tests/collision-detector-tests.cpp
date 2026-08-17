#define _USE_MATH_DEFINES

#include "../src/collision_detector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <sstream>
#include <vector>

namespace Catch {

template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id
            << "," << value.item_id
            << "," << value.sq_distance
            << "," << value.time << ")";
        return tmp.str();
    }
};

}  // namespace Catch

namespace {

using namespace collision_detector;
using Catch::Approx;

constexpr double kEps = 1e-10;

class VectorItemGathererProvider : public ItemGathererProvider {
public:
    VectorItemGathererProvider(std::vector<Item> items, std::vector<Gatherer> gatherers)
        : items_(std::move(items))
        , gatherers_(std::move(gatherers)) {
    }

    size_t ItemsCount() const override {
        return items_.size();
    }
    Item GetItem(size_t idx) const override {
        return items_[idx];
    }
    size_t GatherersCount() const override {
        return gatherers_.size();
    }
    Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

void CheckEvent(const GatheringEvent& event, size_t item_id, size_t gatherer_id,
                 double sq_distance, double time) {
    CHECK(event.item_id == item_id);
    CHECK(event.gatherer_id == gatherer_id);
    CHECK(event.sq_distance == Approx(sq_distance).margin(kEps));
    CHECK(event.time == Approx(time).margin(kEps));
}

}  // namespace

TEST_CASE("No items and no gatherers produce no events", "[collision_detector]") {
    VectorItemGathererProvider provider{{}, {}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("No gatherers produce no events regardless of items", "[collision_detector]") {
    VectorItemGathererProvider provider{{{{1, 2}, 5.}, {{0, 0}, 5.}}, {}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("No items produce no events regardless of gatherers", "[collision_detector]") {
    VectorItemGathererProvider provider{{}, {{{0, 0}, {10, 0}, 1.}}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("Item well within the combined radius is collected", "[collision_detector]") {
    VectorItemGathererProvider provider{{{{5, 0.5}, 0.5}}, {{{0, 0}, {10, 0}, 1.}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CheckEvent(events[0], 0, 0, 0.25, 0.5);
}

TEST_CASE("Item outside the combined radius is not collected", "[collision_detector]") {
    VectorItemGathererProvider provider{{{{5, 3}, 0.1}}, {{{0, 0}, {10, 0}, 1.}}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("Item is only collected when gatherer and item widths are combined",
          "[collision_detector]") {
    // Distance from the item to the segment is 0.5. Neither the gatherer's
    // width (0.3) nor the item's width (0.3) alone reach it, but their sum
    // (0.6) does. This catches implementations that use only one radius.
    VectorItemGathererProvider provider{{{{5, 0.5}, 0.3}}, {{{0, 0}, {10, 0}, 0.3}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CheckEvent(events[0], 0, 0, 0.25, 0.5);
}

TEST_CASE("Item projected before the start of the segment is not collected",
          "[collision_detector]") {
    // The item lies exactly on the infinite line (distance 0) but behind the
    // gatherer's starting point, so it must not be collected.
    VectorItemGathererProvider provider{{{{-5, 0}, 1.}}, {{{0, 0}, {10, 0}, 1.}}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("Item projected beyond the end of the segment is not collected",
          "[collision_detector]") {
    VectorItemGathererProvider provider{{{{15, 0}, 1.}}, {{{0, 0}, {10, 0}, 1.}}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("Item exactly at the start of the segment (t=0) is collected",
          "[collision_detector]") {
    VectorItemGathererProvider provider{{{{0, 0}, 0.}}, {{{0, 0}, {10, 0}, 1.}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CheckEvent(events[0], 0, 0, 0.0, 0.0);
}

TEST_CASE("Item exactly at the end of the segment (t=1) is collected",
          "[collision_detector]") {
    VectorItemGathererProvider provider{{{{10, 0}, 0.}}, {{{0, 0}, {10, 0}, 1.}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CheckEvent(events[0], 0, 0, 0.0, 1.0);
}

TEST_CASE("A stationary gatherer never collects anything", "[collision_detector]") {
    // start_pos == end_pos must be skipped entirely: calling the underlying
    // point/segment math on a degenerate segment would be undefined, and no
    // events should ever be produced for it.
    VectorItemGathererProvider provider{{{{5, 5}, 100.}}, {{{5, 5}, {5, 5}, 100.}}};
    CHECK(FindGatherEvents(provider).empty());
}

TEST_CASE("Events are ordered chronologically by time", "[collision_detector]") {
    // Items are supplied out of chronological order on purpose.
    VectorItemGathererProvider provider{
        {
            {{6, 0}, 0.1},
            {{2, 0}, 0.1},
            {{9, 0}, 0.1},
            {{4, 0}, 0.1},
        },
        {{{0, 0}, {10, 0}, 0.1}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 4);
    for (size_t i = 1; i < events.size(); ++i) {
        CHECK(events[i - 1].time <= events[i].time);
    }
    CheckEvent(events[0], 1, 0, 0.0, 0.2);
    CheckEvent(events[1], 3, 0, 0.0, 0.4);
    CheckEvent(events[2], 0, 0, 0.0, 0.6);
    CheckEvent(events[3], 2, 0, 0.0, 0.9);
}

TEST_CASE("Multiple gatherers can collide with the same item", "[collision_detector]") {
    VectorItemGathererProvider provider{{{{5, 0}, 0.}},
                                         {
                                             {{0, 0}, {10, 0}, 1.},
                                             {{5, -5}, {5, 5}, 1.},
                                         }};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 2);

    bool gatherer0_hit = false;
    bool gatherer1_hit = false;
    for (const auto& event : events) {
        CHECK(event.item_id == 0);
        if (event.gatherer_id == 0) {
            gatherer0_hit = true;
            CHECK(event.time == Approx(0.5).margin(kEps));
        } else if (event.gatherer_id == 1) {
            gatherer1_hit = true;
            CHECK(event.time == Approx(0.5).margin(kEps));
        }
    }
    CHECK(gatherer0_hit);
    CHECK(gatherer1_hit);
}

TEST_CASE("Distance is computed correctly for a diagonal, non-axis-aligned move",
          "[collision_detector]") {
    // The item sits exactly on a diagonal line, off both axes. Implementations
    // that special-case horizontal/vertical movement (or otherwise compute
    // distance using only one coordinate) would report a non-zero distance
    // here instead of the true perpendicular distance of 0.
    VectorItemGathererProvider provider{{{{5, 5}, 0.}}, {{{0, 0}, {10, 10}, 1.}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CheckEvent(events[0], 0, 0, 0.0, 0.5);
}

TEST_CASE("A small combined radius still finds a diagonal collision", "[collision_detector]") {
    // Same geometry as above but with a tight radius: a wrong distance
    // formula would inflate the distance past the radius and miss the event
    // entirely, rather than merely mis-reporting sq_distance.
    VectorItemGathererProvider provider{{{{5, 5}, 0.02}}, {{{0, 0}, {10, 10}, 0.02}}};
    auto events = FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CheckEvent(events[0], 0, 0, 0.0, 0.5);
}
