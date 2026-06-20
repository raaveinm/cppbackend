//
// Created by raaveinm on 6/19/26.
//

#include <cstddef>
#include <iostream>
#include <ostream>
#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::udp;

int main(const int argc, char *argv[]) {
    static constexpr int PORT = 3333;
    static constexpr std::size_t MAX_BUFF_SIZE = 1024;

    if (argc != 2) {
        std::cerr << "Usage: "<< argv[0] <<" <server_ip>" << std::endl;
        return 1;
    }

    try {
        boost::asio::io_context io_context;
        udp::socket socket(io_context, udp::v4());
        boost::system::error_code error;
        const auto endpoint = udp::endpoint(net::ip::make_address(argv[1], error), PORT);
        socket.send_to(net::buffer("I dont care if anyone there"), endpoint);

        std::array<char, MAX_BUFF_SIZE> buffer{};
        udp::endpoint sender_endpoint;
        size_t size = socket.receive_from(net::buffer(buffer), sender_endpoint);
        std::cout << "__server_answer >>> " << std::string_view(buffer.data(), size) << std::endl;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 2;
    }

    return 0;
}
