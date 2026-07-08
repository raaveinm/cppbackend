//
// Created by raaveinm on 6/18/26.
//

#include <iostream>
#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::tcp;

int main() {
    // init port listening
    static constexpr int PORT = 3333;
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), PORT));

    std::cout << "Waiting for connection" << std::endl;

    // delegate to Asio
    boost::system::error_code error;
    tcp::socket socket(io_context);
    acceptor.accept(socket, error);

    if (error) {
        std::cerr << "Can't accept connection" << std::endl;
        return 1;
    }

    // read single line
    net::streambuf stream_buf;
    net::read_until(socket, stream_buf, '\n', error);
    
    if (error) {
        std::cerr << "Can't read data" << std::endl;
        return 2;
    }

    const std::string client_data {
            std::istreambuf_iterator(&stream_buf),
            std::istreambuf_iterator<char>()
    };

    std::cout << "__client_request >>> " << client_data << std::endl;

    socket.write_some(net::buffer("Bonjour\n"), error);

    if (error) {
        std::cerr << "Can't write data" << std::endl;
        return 3;
    }

    return 0;
}