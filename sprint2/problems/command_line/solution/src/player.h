#pragma once
#include <string>
#include <string_view>
#include "tagged.h"
#include "model.h" // Include model.h to get model::Token

#include "model.h"
#include "util/token_generator.h"


namespace model {

    uint64_t Player::player_id_s = 0;

    inline Player::Player(model::Dog* dog, model::GameSession* session)
        : id_{player_id_s++}, dog_{dog}, session_{session} {}

    inline const PlayerId& Player::GetId() const noexcept {
        return id_;
    }

    inline const std::string& Player::GetName() const noexcept {
        return GetDog()->GetName();
    }

    inline Dog* Player::GetDog() const noexcept {
        return dog_;
    }

    inline GameSession* Player::GetSession() const noexcept {
        return session_;
    }

    inline Token PlayerTokens::AddPlayer(const Player& player) {
        const auto token_str = generator_.GenerateToken().get();
        token_to_player_.emplace(Token{token_str}, &player);
        return Token{token_str};
    }

    inline const Player* PlayerTokens::FindPlayerByToken(const Token& token) const {
        if (token_to_player_.contains(token)) {
            return token_to_player_.at(token);
        }
        return nullptr;
    }

    inline Player& Players::Add(Dog* dog, GameSession* session) {
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

} // namespace model