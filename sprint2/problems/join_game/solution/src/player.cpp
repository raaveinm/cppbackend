#include "player.h"
#include "model.h"

#include <boost/uuid/uuid_io.hpp>
#include <sstream>

namespace player {

uint64_t Player::player_id_s = 0;

Player::Player(model::Dog* dog, model::GameSession* session)
    : id_{player_id_s++}, dog_{dog}, session_{session} {}

const PlayerId& Player::GetId() const noexcept {
    return id_;
}

const std::string& Player::GetName() const noexcept {
    return GetDog()->GetName();
}

model::Dog* Player::GetDog() const noexcept {
    return dog_;
}

model::GameSession* Player::GetSession() const noexcept {
    return session_;
}

Token PlayerTokens::AddPlayer(const Player& player) {
    const auto token = [&]() {
        std::stringstream stream;
        stream << std::hex << generator_() << generator_();
        return stream.str();
    }();
    token_to_player_.emplace(Token{token}, &player);
    return Token{token};
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) const {
    if (token_to_player_.contains(token)) {
        return token_to_player_.at(token);
    }
    return nullptr;
}

Player& Players::Add(model::Dog* dog, model::GameSession* session) {
    players_.emplace_back(std::make_unique<Player>(dog, session));
    return *players_.back();
}

const Player* Players::FindByDogIdAndMapId(const model::Dog::Id& dog_id, const model::Map::Id& map_id) const {
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

} // namespace player
