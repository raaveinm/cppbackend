#include "sdk.h"
#include "http_server.h"
#include "json_loader.h"
#include "logging.h"
#include "ticker.h"
#include "request_handler.h"

#include <boost/asio/signal_set.hpp>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>
#include <boost/json.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <optional>
#include <boost/program_options.hpp>


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
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;
    po::options_description desc{"Allowed options"s};

    Args args;
    uint64_t tick_period_val = 0;

    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value<uint64_t>(&tick_period_val)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value<std::string>(&args.config_file)->value_name("file"s), "set config file path")
        ("www-root,w", po::value<std::string>(&args.www_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions");

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
        model::Game game = json_loader::LoadGame(args.config_file, ioc, args.randomize_spawn_points);
        std::filesystem::path static_path(args.www_root);

        // Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // Создаём обработчик запросов
        http_handler::RequestHandler handler{game, static_path, args.tick_period.has_value()};
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
                [&game](std::chrono::milliseconds delta) {
                    game.Tick(delta);
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