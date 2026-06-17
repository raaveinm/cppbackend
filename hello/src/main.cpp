//
// Created by raaveinm on 6/17/26.
//

// Подключим библиотеку Boost.Optional, чтобы убедиться, что Boost подключен успешно
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace net = boost::asio;
namespace sys = boost::system;
using namespace std::chrono;
using namespace std::literals;

int main() {
    // Шаблон boost::optional — прообраз std::optional, используем его здесь для примера
    boost::optional<int> opt;
    std::cout << opt << std::endl;

    net::io_context io;
}

// cd hello/build
// source ../../.venv/bin/activate
// export CMAKE_POLICY_VERSION_MINIMUM=3.5
// conan install .. --build=missing

// cmake .. -DCMAKE_BUILD_TYPE=Release
// cmake --build .

// ./bin/hello
