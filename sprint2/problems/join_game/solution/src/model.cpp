#include "model.h"

#include <stdexcept>
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
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
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
    dogs_.emplace_back(std::make_unique<Dog>(Dog::Id{dog_id_++}, name, Point{0, 0}));
    return dogs_.back().get();
}

std::pair<Player&, Token> Game::AddPlayer(const Map::Id& map_id, const std::string& player_name) {
    auto* map = const_cast<Map*>(FindMap(map_id));
    if (!map) {
        throw std::invalid_argument("Map not found");
    }

    auto* session = FindSession(map_id);
    if (!session) {
        sessions_.emplace_back(std::make_unique<GameSession>(map));
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

Token PlayerTokens::AddPlayer(const Player& player) {
    const auto token_str = generator_.GenerateToken(); // Get std::string
    Token token{token_str}; // Convert to model::Token
    token_to_player_.emplace(token, &player);
    return token;
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) const {
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

const Player* Game::FindPlayerByToken(const Token& token) const {
    return player_tokens_.FindPlayerByToken(token);
}

const std::vector<std::unique_ptr<Player>>& Game::GetPlayers() const {
    return players_.GetPlayers();
}


}  // namespace model