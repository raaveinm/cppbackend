//
// Created by raaveinm on 6/26/26.
//

#include <iostream>
#include <thread>
#include <boost/asio/signal_set.hpp> // System signal handler
#include <boost/asio/steady_timer.hpp>

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

int main() {
    const unsigned threads = std::thread::hardware_concurrency();
    boost::asio::io_context io_context(static_cast<int>(threads));

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM); // Subscribing for signals
    signals.async_wait([&io_context](const boost::system::error_code& error, int signal_number) { // Callback
        if (!error) {
            std::cout << "Signal " << signal_number << " received" << std::endl;
            io_context.stop(); // Killing task
        }
    });

    boost::asio::steady_timer timer(io_context, std::chrono::seconds(30));
    timer.async_wait([](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Timer expired" << std::endl;
        }
    });

    RunWorkers(threads, [&io_context] {
        io_context.run();
    });

    std::cout << "Shutting down" << std::endl;

    return 0;
}