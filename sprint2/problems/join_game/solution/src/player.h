#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <boost/uuid/random_generator.hpp>

#include "model.h"
#include "tagged.h"

namespace model {
class Dog;
class GameSession;
class Map;
}

namespace player {

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using PlayerId = util::Tagged<uint64_t, class PlayerIdTag>;

class Player {
public:
    Player(model::Dog* dog, model::GameSession* session);

    const PlayerId& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    model::Dog* GetDog() const noexcept;
    model::GameSession* GetSession() const noexcept;

private:
    PlayerId id_;
    model::Dog* dog_;
    model::GameSession* session_;
    static uint64_t player_id_s;
};

class PlayerTokens {
public:
    Token AddPlayer(const Player& player);
    const Player* FindPlayerByToken(const Token& token) const;

private:
    using TokenHasher = util::TaggedHasher<Token>;
    std::unordered_map<Token, const Player*, TokenHasher> token_to_player_;
    boost::uuids::random_generator generator_;
};

class Players {
public:
    Player& Add(model::Dog* dog, model::GameSession* session);
    const Player* FindByDogIdAndMapId(const model::Dog::Id& dog_id, const model::Map::Id& map_id) const;
    const Player* FindById(const PlayerId& id) const;

private:
    std::vector<std::unique_ptr<Player>> players_;
};

} // namespace player
