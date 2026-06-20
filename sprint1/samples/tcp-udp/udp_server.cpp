//
// Created by raaveinm on 6/19/26.
//


#include <cstddef>
#include <iostream>
#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::udp;

int main(int argc, char *argv[]) {
    static constexpr int PORT = 3333;
    static constexpr std::size_t MAX_BUFF_SIZE = 1024;

    try {
        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), PORT));

        while (true) {
            std::array<char, MAX_BUFF_SIZE> buffer{};
            udp::endpoint sender_endpoint;
            const auto size = socket.receive_from(boost::asio::buffer(buffer), sender_endpoint);
            std::cout << "__client_request >>> " << std::string_view(buffer.data(), size) << std::endl;
            boost::system::error_code ignored_error;
            socket.send_to(boost::asio::buffer("Bonjour\n"), sender_endpoint, 0, ignored_error);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
