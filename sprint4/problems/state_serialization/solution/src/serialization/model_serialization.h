#pragma once

#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../model/model.h"

namespace model {

template <typename Archive>
void serialize(Archive& ar, Point2D& pt, [[maybe_unused]] const unsigned version) {
    ar & pt.x;
    ar & pt.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed2D& speed, [[maybe_unused]] const unsigned version) {
    ar & speed.x;
    ar & speed.y;
}

template <typename Archive>
void serialize(Archive& ar, LostObject& obj, [[maybe_unused]] const unsigned version) {
    ar & obj.id;
    ar & obj.type;
    ar & obj.pos;
}

template <typename Archive>
void serialize(Archive& ar, BagItem& item, [[maybe_unused]] const unsigned version) {
    ar & item.id;
    ar & item.type;
}

}  // namespace model

namespace state_serialization {

// DogRepr (Dog Representation) - serialized representation of model::Dog.
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(*dog.GetId())
        , name_(dog.GetName())
        , position_(dog.GetPosition())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , bag_(dog.GetBag())
        , score_(dog.GetScore())
        , bag_capacity_(dog.GetBagCapacity()) {
    }

    model::Dog::Id GetId() const {
        return model::Dog::Id{id_};
    }

    const std::string& GetName() const {
        return name_;
    }

    const model::Point2D& GetPosition() const {
        return position_;
    }

    const model::Speed2D& GetSpeed() const {
        return speed_;
    }

    model::Direction GetDirection() const {
        return direction_;
    }

    const model::Dog::Bag& GetBag() const {
        return bag_;
    }

    unsigned GetScore() const {
        return score_;
    }

    size_t GetBagCapacity() const {
        return bag_capacity_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & id_;
        ar & name_;
        ar & position_;
        ar & speed_;
        ar & direction_;
        ar & bag_;
        ar & score_;
        ar & bag_capacity_;
    }

private:
    uint64_t id_ = 0;
    std::string name_;
    model::Point2D position_{};
    model::Speed2D speed_{};
    model::Direction direction_ = model::Direction::NORTH;
    model::Dog::Bag bag_;
    unsigned score_ = 0;
    size_t bag_capacity_ = 0;
};

// SessionRepr - serialized representation of a single map's model::GameSession:
// its dogs and the lost objects currently scattered on the map.
class SessionRepr {
public:
    SessionRepr() = default;

    explicit SessionRepr(const model::GameSession& session)
        : map_id_(*session.GetMap()->GetId())
        , lost_objects_(session.GetLostObjects()) {
        for (const auto& dog : session.GetDogs()) {
            dogs_.emplace_back(*dog);
        }
    }

    const std::string& GetMapId() const {
        return map_id_;
    }

    const std::vector<DogRepr>& GetDogs() const {
        return dogs_;
    }

    const std::unordered_map<uint32_t, model::LostObject>& GetLostObjects() const {
        return lost_objects_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & map_id_;
        ar & dogs_;
        ar & lost_objects_;
    }

private:
    std::string map_id_;
    std::vector<DogRepr> dogs_;
    std::unordered_map<uint32_t, model::LostObject> lost_objects_;
};

// PlayerRepr - serialized representation of a model::Player: which dog on
// which map it controls, plus the auth token that must keep working for
// reconnecting clients after a restore.
class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(const model::Player& player)
        : player_id_(*player.GetId())
        , dog_id_(*player.GetDog()->GetId())
        , map_id_(*player.GetSession()->GetMap()->GetId())
        , token_(*player.GetToken()) {
    }

    uint64_t GetPlayerId() const {
        return player_id_;
    }

    uint64_t GetDogId() const {
        return dog_id_;
    }

    const std::string& GetMapId() const {
        return map_id_;
    }

    const std::string& GetToken() const {
        return token_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & player_id_;
        ar & dog_id_;
        ar & map_id_;
        ar & token_;
    }

private:
    uint64_t player_id_ = 0;
    uint64_t dog_id_ = 0;
    std::string map_id_;
    std::string token_;
};

// GameStateRepr - serialized representation of the whole model::Game: every
// session's dogs & lost objects, plus every active player's token.
class GameStateRepr {
public:
    GameStateRepr() = default;

    explicit GameStateRepr(const model::Game& game) {
        for (const auto& map : game.GetMaps()) {
            if (const auto* session = game.FindSession(map.GetId())) {
                sessions_.emplace_back(*session);
            }
        }
        for (const auto& player : game.GetPlayers()) {
            players_.emplace_back(*player);
        }
    }

    // Applies the saved state onto a game whose maps have already been
    // loaded from the config file. Throws std::runtime_error if the saved
    // state refers to a map or dog that doesn't exist in the current config,
    // which the caller should treat as a corrupted/incompatible save file.
    void Restore(model::Game& game) const {
        for (const auto& session_repr : sessions_) {
            const model::Map::Id map_id{session_repr.GetMapId()};
            auto* session = game.GetOrCreateSession(map_id);
            if (!session) {
                throw std::runtime_error("Saved state references unknown map: " + session_repr.GetMapId());
            }
            for (const auto& dog_repr : session_repr.GetDogs()) {
                session->RestoreDog(dog_repr.GetId(), dog_repr.GetName(), dog_repr.GetPosition(),
                                     dog_repr.GetSpeed(), dog_repr.GetDirection(), dog_repr.GetScore(),
                                     dog_repr.GetBag(), dog_repr.GetBagCapacity());
            }
            for (const auto& [id, object] : session_repr.GetLostObjects()) {
                session->RestoreLostObject(id, object);
            }
        }

        for (const auto& player_repr : players_) {
            const model::Map::Id map_id{player_repr.GetMapId()};
            auto* session = game.FindSession(map_id);
            if (!session) {
                throw std::runtime_error("Saved player references unknown map: " + player_repr.GetMapId());
            }

            model::Dog* dog = nullptr;
            for (const auto& candidate : session->GetDogs()) {
                if (*candidate->GetId() == player_repr.GetDogId()) {
                    dog = candidate.get();
                    break;
                }
            }
            if (!dog) {
                throw std::runtime_error("Saved player references unknown dog id");
            }

            game.RestorePlayer(model::PlayerId{player_repr.GetPlayerId()}, dog, session,
                                model::Token{player_repr.GetToken()});
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & sessions_;
        ar & players_;
    }

private:
    std::vector<SessionRepr> sessions_;
    std::vector<PlayerRepr> players_;
};

}  // namespace state_serialization
