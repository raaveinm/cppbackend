#include "sdk.h"
#include "http_server.h"
#include "json_loader.h"
#include "logging.h"

#include <boost/asio/signal_set.hpp>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>
#include <boost/json.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include "request_handler.h"

namespace net = boost::asio;
namespace sys = boost::system;
namespace json = boost::json;
using namespace std::literals;

namespace {

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
    if (argc != 3) {
        std::cout << "Usage: game_server <game-config-json> <static-root>"sv << std::endl;
        return EXIT_FAILURE;
    }

    logging::Init();

    const auto address = net::ip::make_address("0.0.0.0");
    constexpr net::ip::port_type port = 8080;

    try {
        // 1. Загружаем карту из файла и строим модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        std::filesystem::path static_path(argv[2]);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(static_cast<int>(num_threads));

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4. Создаём обработчик запросов
        http_handler::RequestHandler handler{game, static_path};

        // 5. Запускаем обработчик HTTP-запросов, передав ему модель игры
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, auto&& endpoint, auto&& send) {
            http_handler::LoggingRequestHandler logging_handler{handler};
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(endpoint)>(endpoint), std::forward<decltype(send)>(send));
        });

        // 6. Запускаем обработку асинхронных операций
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