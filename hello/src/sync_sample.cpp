//
// Created by raaveinm on 6/25/26.
//


#include <iomanip>
#include <iostream>
#include <syncstream>
#include <thread>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>

namespace net  = boost::asio;

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


void MakeStrandSample() {
    std::cout << " --- make strand --- " << std::endl;
    constexpr unsigned num_threads = 2;
    boost::asio::io_context io{num_threads};

    boost::asio::steady_timer t1{io, std::chrono::milliseconds(400)};
    boost::asio::steady_timer t2{io, std::chrono::milliseconds(600)};
    boost::asio::steady_timer t3{io, std::chrono::milliseconds(800)};
    boost::asio::steady_timer t4{io, std::chrono::milliseconds(1000)};

    // Эта лямбда-функция вернёт обработчик таймера, который выведет текст и заблокирует
    // текущий поток на 1 секунду
    auto make_timer_handler = [](int index) {
        return [index](boost::system::error_code) {
            std::osyncstream{std::cout} << "Enter #" << index << std::endl;
            // Блокируем текущий поток на 1 секунду, чтобы обработчики,
            // выполняющиеся в разных потоках, пересекались во времени
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::osyncstream{std::cout} << " Exit #" << index << std::endl;
        };
    };

    auto strand1 = make_strand(io);
    auto strand2 = make_strand(io);

    // обработчики таймеров t1 и t2 будут выполнены строго последовательно
    t1.async_wait(bind_executor(strand1, make_timer_handler(1)));
    t2.async_wait(bind_executor(strand1, make_timer_handler(2)));

    // обработчики таймеров t3 и t4 будут выполнены строго последовательно
    t3.async_wait(bind_executor(strand2, make_timer_handler(3)));
    t4.async_wait(bind_executor(strand2, make_timer_handler(4)));

    RunWorkers(num_threads, [&io] {
        io.run();
    });

    std::cout << std::endl;
}


void PostSample() {
    std::cout << " --- post --- " << std::endl;
    using osync = std::osyncstream;

    net::io_context io;
    std::cout << "Eat. Thread id: " << std::this_thread::get_id() << std::endl;

    net::post(io, [] {  // Моем посуду
        osync(std::cout) << "Wash dishes. Thread id: " << std::this_thread::get_id() << std::endl;
    });

    net::post(io, [] {  // Прибираемся на столе
        osync(std::cout) << "Cleanup table. Thread id: " << std::this_thread::get_id() << std::endl;
    });

    net::post(io, [&io] {  // Пылесосим комнату
        osync(std::cout) << "Vacuum-clean room. Thread id: " << std::this_thread::get_id()
                         << std::endl;

        net::post(io, [] {  // После того, как пропылесосили, асинхронно моем пол
            osync(std::cout) << "Wash floor. Thread id: " << std::this_thread::get_id()
                             << std::endl;
        });

        net::post(io, [] {  // Асинхронно опустошаем пылесборник пылесоса
            osync(std::cout) << "Empty vacuum cleaner. Thread id: " << std::this_thread::get_id()
                             << std::endl;
        });
    });

    std::cout << "Work. Thread id: " << std::this_thread::get_id() << std::endl;

    RunWorkers(2, [&io] {  // Асинхронные операции выполняются двумя потоками
        io.run();
    });
    std::cout << "Sleep" << std::endl;
}

void DeferSample() {
    std::cout << " --- defer --- " << std::endl;

    using osync = std::osyncstream;
    using namespace std::chrono;

    // Создаём пул, содержащий два потока
    net::thread_pool tp{2};
    const auto start = steady_clock::now();

    auto print = [start](const char ch) {
        const auto t = duration_cast<duration<double>>(steady_clock::now() - start).count();
        osync(std::cout) << std::fixed << std::setprecision(6)
            << t << "> " << ch << ':' << std::this_thread::get_id() << std::endl;
    };

    net::post(tp, [&tp, print] {
        print('A');

        net::defer(tp, [print] { print('B'); });
        net::defer(tp, [print] { print('C'); });
        net::defer(tp, [print] { print('D'); });

        // Засыпаем, чтобы дать шанс другим потокам сделать свою работу
        std::this_thread::sleep_for(seconds(1));
    });

    // Дожидаемся окончания работы потоков
    tp.wait();
}

void DispatchSample() {
    std::cout << " --- dispatch --- " << std::endl;

    net::io_context io;

    net::post(io, [&io] {
        std::cout << 'A';
        net::post(io, [] {
            std::cout << 'B';
        });
        std::cout << 'C';
    });

    net::dispatch(io, [&io] {
        std::cout << 'D';
        net::post(io, [] {
            std::cout << 'E';
        });
        net::defer(io, [] {
            std::cout << 'F';
        });
        net::dispatch(io, [] {
            std::cout << 'G';
        });
        std::cout << 'H';
    });

    std::cout << 'I';
    io.run();
    std::cout << 'J';
}

int main() {
    // MakeStrandSample();
    // PostSample();
    // DeferSample();
    DispatchSample();
}
