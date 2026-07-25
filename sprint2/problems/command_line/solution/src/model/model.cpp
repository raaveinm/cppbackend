#include "model.h"

#include <stdexcept>
#include <algorithm>

namespace model {
using namespace std::literals;

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
    if (road.IsHorizontal()) {
        auto start = road.GetStart();
        auto end = road.GetEnd();
        double x1 = static_cast<double>(start.x);
        double x2 = static_cast<double>(end.x);
        double y = static_cast<double>(start.y);

        horizontal_road_boundaries_.push_back({
            std::min(x1, x2) - 0.4,
            y - 0.4,
            std::max(x1, x2) + 0.4,
            y + 0.4
        });
    } else { // Vertical
        auto start = road.GetStart();
        auto end = road.GetEnd();
        double y1 = static_cast<double>(start.y);
        double y2 = static_cast<double>(end.y);
        double x = static_cast<double>(start.x);

        vertical_road_boundaries_.push_back({
            x - 0.4,
            std::min(y1, y2) - 0.4,
            x + 0.4,
            std::max(y1, y2) + 0.4
        });
    }
}

uint64_t Player::player_id_s = 0;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

Point2D Map::GetRandomPointOnRoads() const {
    if (roads_.empty()) {
        return {0.0, 0.0};
    }

    thread_local std::mt19937 generator{std::random_device{}()};

    std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
    const auto& road = roads_[road_dist(generator)];

    if (road.IsHorizontal()) {
        const double min_x = std::min(road.GetStart().x, road.GetEnd().x);
        const double max_x = std::max(road.GetStart().x, road.GetEnd().x);
        std::uniform_real_distribution<double> coord_dist(min_x, max_x);
        return {coord_dist(generator), static_cast<double>(road.GetStart().y)};
    } else {
        const double min_y = std::min(road.GetStart().y, road.GetEnd().y);
        const double max_y = std::max(road.GetStart().y, road.GetEnd().y);
        std::uniform_real_distribution<double> coord_dist(min_y, max_y);
        return {static_cast<double>(road.GetStart().x), coord_dist(generator)};
    }
}

bool Map::IsOnRoad(Point2D pt) const {
    for (const auto& boundary : horizontal_road_boundaries_) {
        if (pt.x >= boundary.x1 && pt.x <= boundary.x2 &&
            pt.y >= boundary.y1 && pt.y <= boundary.y2) {
            return true;
        }
    }
    for (const auto& boundary : vertical_road_boundaries_) {
        if (pt.x >= boundary.x1 && pt.x <= boundary.x2 &&
            pt.y >= boundary.y1 && pt.y <= boundary.y2) {
            return true;
        }
    }
    return false;
}

RoadBoundary Map::GetAllowedBoundaries(Point2D pt, Direction dir) const {
    std::vector<RoadBoundary> current_roads;

    auto check_and_add = [&](const std::vector<RoadBoundary>& boundaries) {
        for (const auto& b : boundaries) {
            if (pt.x >= b.x1 - 1e-7 && pt.x <= b.x2 + 1e-7 &&
                pt.y >= b.y1 - 1e-7 && pt.y <= b.y2 + 1e-7) {
                current_roads.push_back(b);
            }
        }
    };

    check_and_add(horizontal_road_boundaries_);
    check_and_add(vertical_road_boundaries_);

    if (current_roads.empty()) {
        return {pt.x, pt.y, pt.x, pt.y};
    }

    RoadBoundary result = current_roads.front();
    for (const auto& b : current_roads) {
        result.x1 = std::min(result.x1, b.x1);
        result.x2 = std::max(result.x2, b.x2);
        result.y1 = std::min(result.y1, b.y1);
        result.y2 = std::max(result.y2, b.y2);
    }

    bool expanded = true;
    while (expanded) {
        expanded = false;

        if (dir == Direction::NORTH || dir == Direction::SOUTH) {
            for (const auto& b : vertical_road_boundaries_) {
                if (std::abs((b.x1 + b.x2) / 2.0 - pt.x) <= 0.4 + 1e-7) {
                    if (b.y1 <= result.y2 + 1e-7 && b.y2 >= result.y1 - 1e-7) {
                        if (b.y1 < result.y1 - 1e-7) { result.y1 = b.y1; expanded = true; }
                        if (b.y2 > result.y2 + 1e-7) { result.y2 = b.y2; expanded = true; }
                    }
                }
            }
        } else {
            for (const auto& b : horizontal_road_boundaries_) {
                if (std::abs((b.y1 + b.y2) / 2.0 - pt.y) <= 0.4 + 1e-7) {
                    if (b.x1 <= result.x2 + 1e-7 && b.x2 >= result.x1 - 1e-7) {
                        if (b.x1 < result.x1 - 1e-7) { result.x1 = b.x1; expanded = true; }
                        if (b.x2 > result.x2 + 1e-7) { result.x2 = b.x2; expanded = true; }
                    }
                }
            }
        }
    }

    return result;
}

void Dog::Tick(const std::chrono::milliseconds delta_t) {
    const auto delta_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(delta_t).count();

    if (abs(speed_.x) < 1e-10 && abs(speed_.y) < 1e-10)
        return;

    Point2D proposed_pos{};
    proposed_pos.x = position_.x + speed_.x * delta_seconds;
    proposed_pos.y = position_.y + speed_.y * delta_seconds;

    const auto* map = session_->GetMap();
    auto boundary = map->GetAllowedBoundaries(position_, direction_);

    Point2D clamped_pos = proposed_pos;
    clamped_pos.x = std::clamp(proposed_pos.x, boundary.x1, boundary.x2);
    clamped_pos.y = std::clamp(proposed_pos.y, boundary.y1, boundary.y2);

    if (std::abs(clamped_pos.x - proposed_pos.x) > 1e-10 || std::abs(clamped_pos.y - proposed_pos.y) > 1e-10) {
        speed_ = {0.0, 0.0};
    }

    position_ = clamped_pos;
}

void GameSession::Tick(std::chrono::milliseconds delta_t) {
    Dispatch([this, delta_t]() {
        for (auto& dog : dogs_) {
            dog->Tick(delta_t);
        }
    });
}

void Game::AddMap(Map map) {
    map.SetDefaultDogSpeed(default_dog_speed_);
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void Game::Tick(std::chrono::milliseconds delta_t) {
    for (auto& session : sessions_) {
        session->Tick(delta_t);
    }
}

Dog* GameSession::AddDog(const std::string& name) {
    Point2D pos{};
    if (randomize_spawn_points_) {
        pos = map_->GetRandomPointOnRoads();
    } else {
        auto start_point = map_->GetRoads().front().GetStart();
        pos = {static_cast<double>(start_point.x), static_cast<double>(start_point.y)};
    }
    dogs_.emplace_back(std::make_unique<Dog>(Dog::Id{dog_id_++}, name, pos, this));
    return dogs_.back().get();
}

std::pair<Player&, Token> Game::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    auto* map = const_cast<Map*>(FindMap(map_id));
    if (!map) {
        throw std::invalid_argument("Map not found");
    }

    auto* session = FindSession(map_id);
    if (!session) {
        sessions_.emplace_back(std::make_unique<GameSession>(map, ioc_, randomize_spawn_points_));
        map_id_to_session_index_[map_id] = sessions_.size() - 1;
        session = sessions_.back().get();
    }

    auto* dog = session->AddDog(player_name);
    auto& player = players_.Add(dog, session);
    auto token = player_tokens_.AddPlayer(player);

    return {player, token};
}

Player::Player(Dog* dog, GameSession* session)
    : id_{player_id_s++}, dog_{dog}, session_{session} {}

const PlayerId& Player::GetId() const noexcept {
    return id_;
}

const std::string& Player::GetName() const noexcept {
    return GetDog()->GetName();
}

Dog* Player::GetDog() const noexcept {
    return dog_;
}

GameSession* Player::GetSession() const noexcept {
    return session_;
}

Token PlayerTokens::AddPlayer(Player& player) {
    const auto token_str = generator_.GenerateToken();
    Token token{token_str}; // Convert to model::Token
    token_to_player_.emplace(token, &player);
    return token;
}

Player* PlayerTokens::FindPlayerByToken(const Token& token) {
    if (token_to_player_.contains(token)) {
        return token_to_player_.at(token);
    }
    return nullptr;
}

Player& Players::Add(Dog* dog, GameSession* session) {
    players_.emplace_back(std::make_unique<Player>(dog, session));
    return *players_.back();
}

const Player* Players::FindByDogIdAndMapId(const Dog::Id& dog_id, const Map::Id& map_id) const {
    for (const auto& player : players_) {
        if (player->GetDog()->GetId() == dog_id && player->GetSession()->GetMap()->GetId() == map_id) {
            return player.get();
        }
    }
    return nullptr;
}

const Player* Players::FindById(const PlayerId& id) const {
    for (const auto& player : players_) {
        if (player->GetId() == id) {
            return player.get();
        }
    }
    return nullptr;
}

Player* Game::FindPlayerByToken(const Token& token) {
    return player_tokens_.FindPlayerByToken(token);
}

const std::vector<std::unique_ptr<Player>>& Game::GetPlayers() const {
    return players_.GetPlayers();
}


}  // namespace model
