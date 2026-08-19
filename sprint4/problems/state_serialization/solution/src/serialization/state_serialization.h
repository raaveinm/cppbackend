#pragma once

#include <boost/log/trivial.hpp>
#include <chrono>
#include <filesystem>

#include "../model/model.h"

namespace state_serialization {

// Serializes the game state to `path` atomically: the state is first written
// to a temporary file next to `path`, then moved into place with
// std::filesystem::rename so a reader/crash never observes a partial file.
void SaveGameState(const model::Game& game, const std::filesystem::path& path);

// Deserializes the game state from `path` and applies it onto `game`, whose
// maps must already be populated from the config file. Throws
// std::runtime_error (or a derived boost::archive exception) if the file is
// missing, corrupted, or refers to unknown maps/dogs.
void LoadGameState(model::Game& game, const std::filesystem::path& path);

// Tracks elapsed game time and triggers a save once it reaches the
// configured period. Shared between the automatic ticker and the
// /api/v1/game/tick endpoint so both paths trigger autosaves consistently.
class StatePersister {
public:
    StatePersister(model::Game& game, std::filesystem::path path, std::chrono::milliseconds period)
        : game_{game}
        , path_{std::move(path)}
        , period_{period} {
    }

    // Call once after every game.Tick(delta) with that same delta. Never
    // throws: a failed autosave is logged and simply retried on next tick.
    void MaybeSave(std::chrono::milliseconds delta) {
        elapsed_since_save_ += delta;
        if (elapsed_since_save_ < period_) {
            return;
        }
        elapsed_since_save_ = std::chrono::milliseconds::zero();
        try {
            Save();
        } catch (const std::exception& e) {
            BOOST_LOG_TRIVIAL(error) << "Periodic state autosave failed: " << e.what();
        }
    }

    // Saves unconditionally, e.g. on server shutdown. Propagates failures so
    // the caller can decide how to react.
    void Save() {
        SaveGameState(game_, path_);
    }

private:
    model::Game& game_;
    std::filesystem::path path_;
    std::chrono::milliseconds period_;
    std::chrono::milliseconds elapsed_since_save_{0};
};

}  // namespace state_serialization
