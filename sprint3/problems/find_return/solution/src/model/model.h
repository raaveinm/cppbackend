#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <random>
#include <boost/uuid/random_generator.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include "../util/tagged.h"
#include "../util/token_generator.h"
#include "../loot_generator.h"
#include "../geom.h"
#include "../collision_detector.h"

namespace model {

namespace net = boost::asio;

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using PlayerId = util::Tagged<uint64_t, class PlayerIdTag>;

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct RoadBoundary {
    double x1, y1, x2, y2;
};

struct Offset {
    Dimension dx, dy;
};

struct Point2D {
    double x, y;
};

struct Speed2D {
    double x, y;
};

struct LostObject {
    uint32_t id;
    size_t type;
    Point2D pos;
};

struct BagItem {
    uint32_t id;
    size_t type;
};

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, const Point start, const Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(const Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road);

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    Point2D GetRandomPointOnRoads() const;
    size_t GetRandomLootType() const;

    double GetDogSpeed() const {
        return dog_speed_.value_or(default_dog_speed_);
    }

    void SetDefaultDogSpeed(double speed) {
        default_dog_speed_ = speed;
    }

    void SetDogSpeed(double speed) {
        dog_speed_ = speed;
    }

    void SetLootTypeCount(size_t count) {
        loot_type_count_ = count;
    }

    size_t GetLootTypeCount() const {
        return loot_type_count_;
    }

    size_t GetBagCapacity() const {
        return bag_capacity_.value_or(default_bag_capacity_);
    }

    void SetDefaultBagCapacity(size_t capacity) {
        default_bag_capacity_ = capacity;
    }

    void SetBagCapacity(size_t capacity) {
        bag_capacity_ = capacity;
    }

    bool IsOnRoad(Point2D pt) const;
    RoadBoundary GetAllowedBoundaries(Point2D pt, Direction dir) const;

    const std::vector<RoadBoundary>& GetHorizontalRoadBoundaries() const {
        return horizontal_road_boundaries_;
    }

    const std::vector<RoadBoundary>& GetVerticalRoadBoundaries() const {
        return vertical_road_boundaries_;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::vector<RoadBoundary> horizontal_road_boundaries_;
    std::vector<RoadBoundary> vertical_road_boundaries_;
    std::optional<double> dog_speed_;
    double default_dog_speed_ = 1.0;
    size_t loot_type_count_ = 0;
    std::optional<size_t> bag_capacity_;
    size_t default_bag_capacity_ = 3;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class GameSession;
class Player;

class Dog {
public:
    using Id = util::Tagged<uint64_t, Dog>;
    using Bag = std::vector<BagItem>;

    Dog(Id id, std::string name, Point2D position, GameSession* session, size_t bag_capacity) noexcept
        : id_{id}, name_{std::move(name)}, position_{position}, session_{session}, bag_capacity_{bag_capacity} {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Point2D& GetPosition() const noexcept { return position_; }
    const Speed2D& GetSpeed() const noexcept { return speed_; }
    Direction GetDirection() const noexcept { return direction_; }
    const Bag& GetBag() const noexcept { return bag_; }

    void SetPosition(Point2D position) { position_ = position; }
    void SetSpeed(Speed2D speed) { speed_ = speed; }
    void SetDirection(Direction direction) { direction_ = direction; }

    bool IsBagFull() const noexcept { return bag_.size() >= bag_capacity_; }

    bool CollectItem(BagItem item) {
        if (IsBagFull()) {
            return false;
        }
        bag_.push_back(item);
        return true;
    }

    void EmptyBag() { bag_.clear(); }

    void Tick(std::chrono::milliseconds delta_t);

private:
    Id id_;
    std::string name_;
    Point2D position_;
    Speed2D speed_ = {0.0, 0.0};
    Direction direction_ = Direction::NORTH;
    GameSession* session_;
    Bag bag_;
    size_t bag_capacity_;
};


class GameSession {
public:
    using Dogs = std::vector<std::unique_ptr<Dog>>;
    using LostObjects = std::unordered_map<uint32_t, LostObject>;
    using LootGenerator = loot_gen::LootGenerator;

    explicit GameSession(const Map* map, net::io_context& ioc, bool randomize_spawn_points,
                         LootGenerator::RandomGenerator random_generator,
                         const std::optional<std::pair<double, double>>& loot_generator_config)
        : map_{map}, strand_{net::make_strand(ioc)}, randomize_spawn_points_{randomize_spawn_points} {
        if (loot_generator_config) {
            loot_generator_ = std::make_unique<loot_gen::LootGenerator>(
                std::chrono::milliseconds(static_cast<long long>(loot_generator_config->first * 1000)),
                loot_generator_config->second,
                random_generator);
        }
    }

    const Map* GetMap() const noexcept { return map_; }
    const Dogs& GetDogs() const { return dogs_; }
    Dog* AddDog(const std::string& name);

    void Tick(std::chrono::milliseconds delta_t, bool sync = false);

    template <typename Handler>
    void Dispatch(Handler&& handler) {
        net::dispatch(strand_, std::forward<Handler>(handler));
    }

    const LostObjects& GetLostObjects() const {
        return lost_objects_;
    }

private:
    void ProcessCollisions(const std::vector<Point2D>& start_positions);

    const Map* map_;
    Dogs dogs_;
    uint64_t dog_id_ = 0;
    net::strand<net::io_context::executor_type> strand_;
    bool randomize_spawn_points_ = false;
    LostObjects lost_objects_;
    uint32_t lost_object_id_ = 0;
    std::unique_ptr<loot_gen::LootGenerator> loot_generator_;
};

class Player {
public:
    Player(Dog* dog, GameSession* session);

    const PlayerId& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    Dog* GetDog() const noexcept;
    GameSession* GetSession() const noexcept;

private:
    PlayerId id_;
    Dog* dog_;
    GameSession* session_;
    static uint64_t player_id_s;
};

class PlayerTokens {
public:
    Token AddPlayer(Player& player);
    Player* FindPlayerByToken(const Token& token);

private:
    using TokenHasher = util::TaggedHasher<Token>;
    std::unordered_map<Token, Player*, TokenHasher> token_to_player_;
    util::TokenGenerator generator_;
};

class Players {
public:
    Player& Add(Dog* dog, GameSession* session);
    const Player* FindByDogIdAndMapId(const Dog::Id& dog_id, const Map::Id& map_id) const;
    const Player* FindById(const PlayerId& id) const;
    const std::vector<std::unique_ptr<Player>>& GetPlayers() const { return players_; }

private:
    std::vector<std::unique_ptr<Player>> players_;
};


class Game {
public:
    using Maps = std::vector<Map>;
    using GameSessions = std::vector<std::unique_ptr<GameSession>>;
    using LootGenerator = loot_gen::LootGenerator;

    explicit Game(net::io_context& ioc, bool randomize_spawn_points, LootGenerator::RandomGenerator random_generator)
        : ioc_{ioc}, randomize_spawn_points_{randomize_spawn_points}, random_generator_{std::move(random_generator)} {}

    void AddMap(Map map);
    void Tick(std::chrono::milliseconds delta_t);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        const auto it = map_id_to_index_.find(id);
        return (it != map_id_to_index_.end()) ? &maps_[it->second] : nullptr;
    }

    GameSession* FindSession(const Map::Id& id) noexcept {
        if (const auto it = map_id_to_session_index_.find(id); it != map_id_to_session_index_.end()) {
            return sessions_.at(it->second).get();
        }
        return nullptr;
    }

    std::pair<Player&, Token> AddPlayer(const Map::Id& map_id, const std::string& player_name);

    Player* FindPlayerByToken(const Token& token);
    const std::vector<std::unique_ptr<Player>>& GetPlayers() const;

    double GetDefaultDogSpeed() const noexcept {
        return default_dog_speed_;
    }

    void SetDefaultDogSpeed(double speed) noexcept {
        default_dog_speed_ = speed;
    }

    size_t GetDefaultBagCapacity() const noexcept {
        return default_bag_capacity_;
    }

    void SetDefaultBagCapacity(size_t capacity) noexcept {
        default_bag_capacity_ = capacity;
    }

    void SetLootGeneratorConfig(double period, double probability) {
        loot_generator_config_ = {period, probability};
    }

    const auto& GetLootGeneratorConfig() const {
        return loot_generator_config_;
    }


private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessionIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    net::io_context& ioc_;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    GameSessions sessions_;
    MapIdToSessionIndex map_id_to_session_index_;

    Players players_;
    PlayerTokens player_tokens_;
    double default_dog_speed_ = 1.0;
    size_t default_bag_capacity_ = 3;
    bool randomize_spawn_points_ = false;
    std::optional<std::pair<double, double>> loot_generator_config_;
    LootGenerator::RandomGenerator random_generator_;
};

}  // namespace model