//
// Created by raaveinm on 6/20/26.
//

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <iostream>

namespace net = boost::asio;
using boost::asio::ip::tcp;

int main() {
    // initialization
    net::io_context io_context;
    const auto ADDRESS = net::ip::make_address("0.0.0.0");
    constexpr unsigned short PORT = 8080;
    tcp::acceptor acceptor(io_context, {ADDRESS, PORT});

    // connection
    std::cout << "waiting for connection..." << std::endl;
    tcp::socket socket(io_context);
    acceptor.accept(socket);
    std::cout << "connection recieved" << std::endl;

    // response
    constexpr std::string_view response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n\r\n"
        "Hello";

    net::write(socket, net::buffer(response));

    return 0;
}