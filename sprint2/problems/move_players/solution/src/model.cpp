#include "model.h"

#include <stdexcept>
#include <algorithm>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>

namespace model {
using namespace std::literals;

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

    // Each thread will get its own isolated instance of the generator
    thread_local std::mt19937 generator{std::random_device{}()};

    std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
    const auto& road = roads_[road_dist(generator)];

    if (road.IsHorizontal()) {
        double min_x = std::min(road.GetStart().x, road.GetEnd().x);
        double max_x = std::max(road.GetStart().x, road.GetEnd().x);
        std::uniform_real_distribution<double> coord_dist(min_x, max_x);
        return {coord_dist(generator), static_cast<double>(road.GetStart().y)};
    } else {
        double min_y = std::min(road.GetStart().y, road.GetEnd().y);
        double max_y = std::max(road.GetStart().y, road.GetEnd().y);
        std::uniform_real_distribution<double> coord_dist(min_y, max_y);
        return {static_cast<double>(road.GetStart().x), coord_dist(generator)};
    }
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

Dog* GameSession::AddDog(const std::string& name) {
    auto pos = map_->GetRandomPointOnRoads();
    dogs_.emplace_back(std::make_unique<Dog>(Dog::Id{dog_id_++}, name, pos));
    return dogs_.back().get();
}

std::pair<Player&, Token> Game::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    auto* map = const_cast<Map*>(FindMap(map_id));
    if (!map) {
        throw std::invalid_argument("Map not found");
    }

    auto* session = FindSession(map_id);
    if (!session) {
        sessions_.emplace_back(std::make_unique<GameSession>(map, ioc_));
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