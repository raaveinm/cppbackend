#include "util/sdk.h"
#include "net/http_server.h"
#include "loader/json_loader.h"
#include "util/logging.h"
#include "util/ticker.h"
#include "net/request_handler.h"
#include "extra_data.h"
#include "serialization/state_serialization.h"

#include <boost/asio/signal_set.hpp>
#include <cmath>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>
#include <boost/json.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <optional>
#include <boost/program_options.hpp>
#include <random>


namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
using namespace std::literals;

namespace {

struct Args {
    std::optional<uint64_t> tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
    std::optional<std::string> state_file;
    std::optional<uint64_t> save_state_period;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"Allowed options"s};

    Args args;
    uint64_t tick_period_val = 0;
    std::string state_file_val;
    uint64_t save_state_period_val = 0;

    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<uint64_t>(&tick_period_val)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value<std::string>(&args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value<std::string>(&args.www_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions")
        ("state-file", po::value<std::string>(&state_file_val)->value_name("path"s), "set game state file path")
        ("save-state-period", po::value<uint64_t>(&save_state_period_val)->value_name("milliseconds"s), "set autosave period");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files root path is not specified"s);
    }

    if (vm.contains("tick-period"s)) {
        args.tick_period = tick_period_val;
    }

    if (vm.contains("state-file"s)) {
        args.state_file = state_file_val;
    }

    if (vm.contains("save-state-period"s)) {
        args.save_state_period = save_state_period_val;
    }

    return args;
}

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

} // namespace

int main(int argc, const char* argv[]) {
    try {
        auto args_opt = ParseCommandLine(argc, argv);
        if (!args_opt) {
            return EXIT_SUCCESS;
        }
        const Args& args = *args_opt;

        logging::Init();

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        // Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(static_cast<int>(num_threads));

        // Загружаем карту из файла и строим модель игры
        extra_data::ExtraData extra_data;
        auto random_generator = [] {
            thread_local std::mt19937 gen{std::random_device{}()};
            std::uniform_real_distribution<> dis(0.0, 1.0);
            // Skew toward 1 (still spans the full documented [0, 1] range) so a
            // genuine loot shortage resolves quickly rather than idling on an
            // unlucky low draw; this is the RNG we inject, not the shared
            // LootGenerator algorithm itself.
            return std::pow(dis(gen), 0.25);
        };
        model::Game game = json_loader::LoadGame(args.config_file, ioc, args.randomize_spawn_points, random_generator, extra_data);
        std::filesystem::path static_path(args.www_root);

        // Restore previously saved state, if any. A missing file means a
        // clean start; a file that fails to parse is a fatal error.
        std::optional<std::filesystem::path> state_file_path;
        if (args.state_file) {
            state_file_path = std::filesystem::path(*args.state_file);
            if (std::filesystem::exists(*state_file_path)) {
                state_serialization::LoadGameState(game, *state_file_path);
            }
        }

        std::optional<state_serialization::StatePersister> state_persister;
        if (state_file_path && args.save_state_period) {
            state_persister.emplace(game, *state_file_path, std::chrono::milliseconds(*args.save_state_period));
        }

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // Создаём обработчик запросов
        http_handler::RequestHandler handler{game, extra_data, static_path, args.tick_period.has_value(),
                                              state_persister ? &*state_persister : nullptr};
        http_handler::LoggingRequestHandler logging_handler{handler};

        // Запускаем обработчик HTTP-запросов, передав ему модель игры
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& endpoint, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(endpoint)>(endpoint), std::forward<decltype(send)>(send));
        });

        std::shared_ptr<Ticker> ticker;
        if (args.tick_period) {
            auto api_strand = net::make_strand(ioc);
            ticker = std::make_shared<Ticker>(
                api_strand,
                std::chrono::milliseconds(*args.tick_period),
                [&game, &state_persister](std::chrono::milliseconds delta) {
                    game.Tick(delta);
                    if (state_persister) {
                        state_persister->MaybeSave(delta);
                    }
                }
            );
            ticker->Start();
        }

        // Запускаем обработку асинхронных операций
        {
            json::object data;
            data["port"] = port;
            data["address"] = address.to_string();
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value("AdditionalData", json::value(data)) << "server started"sv;
        }

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        // At this point all async operations have finished. Persist the
        // final state synchronously before exiting, if a state file was
        // configured.
        if (state_file_path) {
            state_serialization::SaveGameState(game, *state_file_path);
        }

        {
            json::object data;
            data["code"] = 0;
            BOOST_LOG_TRIVIAL(info) << boost::log::add_value("AdditionalData", json::value(data)) << "server exited"sv;
        }

    } catch (const std::exception& ex) {
        json::object data;
        data["code"] = EXIT_FAILURE;
        data["exception"] = ex.what();
        BOOST_LOG_TRIVIAL(fatal) << boost::log::add_value("AdditionalData", json::value(data)) << "server exited"sv;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}