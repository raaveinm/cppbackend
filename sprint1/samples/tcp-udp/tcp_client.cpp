//
// Created by raaveinm on 6/18/26.
//
#include <iostream>
#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::tcp;

int main(const int argc, char *argv[]) {
    static constexpr int PORT = 3333;

    if (argc != 2) {
        std::cerr << "Usage: "<< argv[0] <<" <server_ip>" << std::endl;
        return 1;
    }

    // create endpoint data
    boost::system::error_code error;
    const auto endpoint = tcp::endpoint(net::ip::make_address(argv[1], error), PORT);

    if (error) {
        std::cerr << "Could not connect to " << endpoint << std::endl;
        return 2;
    }

    // connect to server
    net::io_context io_context;
    tcp::socket socket(io_context);
    socket.connect(endpoint, error);

    if (error) {
        std::cerr << "Could not connect to " << endpoint << std::endl;
        return 3;
    }

    // boost::asio::write - await all data to be sent
    // boost::asio::write_some - at least one byte sent
    // boost::asio::async_write - add data to queue and forget about it
    socket.write_some(net::buffer("Bonjour, is anyone there?\n"), error);

    if (error) {
        std::cerr << "Can't write data" << std::endl;
        return 4;
    }

    net::streambuf stream_buf;
    net::read_until(socket, stream_buf, '\n', error);
    const std::string server_answer{
            std::istreambuf_iterator(&stream_buf),
            std::istreambuf_iterator<char>()
    };

    if (error) {
        std::cerr << "Can't read data" << std::endl;
        return 5;
    }

    std::cout << "__server_answer >>> " << server_answer << std::endl;

    return 0;
}
