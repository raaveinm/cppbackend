#pragma once

#include <filesystem>
#include <memory>
#include <boost/asio/io_context.hpp>

#include "../model/model.h"
#include "../extra_data.h"

namespace json_loader {

namespace net = boost::asio;

std::unique_ptr<model::Game> LoadGame(const std::filesystem::path& json_path, net::io_context& ioc, bool randomize_spawn_points, model::Game::LootGenerator::RandomGenerator random_generator, extra_data::ExtraData& extra_data);

}  // namespace json_loader