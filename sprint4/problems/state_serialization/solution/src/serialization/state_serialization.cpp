#include "state_serialization.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>
#include <stdexcept>

#include "model_serialization.h"

namespace state_serialization {

void SaveGameState(const model::Game& game, const std::filesystem::path& path) {
    const GameStateRepr repr{game};

    auto tmp_path = path;
    tmp_path += ".tmp";

    {
        std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Failed to open temporary state file for writing: " + tmp_path.string());
        }
        boost::archive::text_oarchive archive{out};
        archive << repr;
        if (!out) {
            throw std::runtime_error("Failed to write state file: " + tmp_path.string());
        }
    }

    std::filesystem::rename(tmp_path, path);
}

void LoadGameState(model::Game& game, const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open state file for reading: " + path.string());
    }

    GameStateRepr repr;
    try {
        boost::archive::text_iarchive archive{in};
        archive >> repr;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse state file " + path.string() + ": " + e.what());
    }

    repr.Restore(game);
}

}  // namespace state_serialization
