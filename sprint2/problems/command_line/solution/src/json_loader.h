#pragma once

#include <filesystem>

#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path, net::io_context& ioc, bool randomize_spawn_points);

}  // namespace json_loader
