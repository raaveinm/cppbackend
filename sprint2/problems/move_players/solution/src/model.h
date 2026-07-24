#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <random>
#include <boost/uuid/random_generator.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

namespace net = boost::asio;

#include "tagged.h"
#include "util/token_generator.h"

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

struct Offset {
    Dimension dx, dy;
};

struct Point2D {
    double x, y;
};

struct Speed2D {
    double x, y;
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

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    Point2D GetRandomPointOnRoads() const;

    double GetDogSpeed() const {
        return dog_speed_.value_or(default_dog_speed_);
    }

    void SetDefaultDogSpeed(double speed) {
        default_dog_speed_ = speed;
    }

    void SetDogSpeed(double speed) {
        dog_speed_ = speed;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    std::optional<double> dog_speed_;
    double default_dog_speed_ = 1.0;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class GameSession;
class Player;

class Dog {
public:
    using Id = util::Tagged<uint64_t, Dog>;

    Dog(Id id, std::string name, Point2D position) noexcept
        : id_{id}, name_{std::move(name)}, position_{position} {}

    const Id& GetId() const noexcept { return id_; }
    const std::string& GetName() const noexcept { return name_; }
    const Point2D& GetPosition() const noexcept { return position_; }
    const Speed2D& GetSpeed() const noexcept { return speed_; }
    Direction GetDirection() const noexcept { return direction_; }

    void SetPosition(Point2D position) { position_ = position; }
    void SetSpeed(Speed2D speed) { speed_ = speed; }
    void SetDirection(Direction direction) { direction_ = direction; }

private:
    Id id_;
    std::string name_;
    Point2D position_;
    Speed2D speed_ = {0.0, 0.0};
    Direction direction_ = Direction::NORTH;
};


class GameSession {
public:
    using Dogs = std::vector<std::unique_ptr<Dog>>;

    explicit GameSession(const Map* map, net::io_context& ioc)
        : map_{map}, strand_{net::make_strand(ioc)} {}

    const Map* GetMap() const noexcept { return map_; }
    const Dogs& GetDogs() const { return dogs_; }
    Dog* AddDog(const std::string& name);

    template <typename Handler>
    void Dispatch(Handler&& handler) {
        net::dispatch(strand_, std::forward<Handler>(handler));
    }

private:
    const Map* map_;
    Dogs dogs_;
    uint64_t dog_id_ = 0;
    net::strand<net::io_context::executor_type> strand_;
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

    explicit Game(net::io_context& ioc) : ioc_{ioc} {}

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (const auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
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
};

}  // namespace model