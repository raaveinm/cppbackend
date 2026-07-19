#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}

int main(const int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-dir>"sv << std::endl;
        return EXIT_FAILURE;
    }

    InitBoostLog();

    try {
        std::filesystem::path static_path(argv[2]);
        model::Game game = json_loader::LoadGame(argv[1]);

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(static_cast<int>(num_threads));

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        const auto address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

        http_handler::RequestHandler handler{game, std::move(static_path)};
        http_handler::LoggingRequestHandler logging_handler{handler};

        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& send, auto&& remote_ip) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send), std::forward<decltype(remote_ip)>(remote_ip));
        });

        json::value started_data = {
            {"port", port},
            {"address", address.to_string()}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, started_data) << "server started";

        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });

        json::value exited_data = {
            {"code", 0}
        };
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, exited_data) << "server exited";

    } catch (const std::exception& ex) {
        json::value exited_data = {
            {"code", EXIT_FAILURE},
            {"exception", ex.what()}
        };
        BOOST_LOG_TRIVIAL(fatal) << logging::add_value(additional_data, exited_data) << "server exited";
        return EXIT_FAILURE;
    }
}