#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"
#include <iostream>
#include <string>
#include <vector>
#include <string_view>
#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::udp;

void StartServer(const uint16_t port) {
    try {
        net::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        Player player(ma_format_u8, 1);
        std::vector<char> buffer(70000);

        while (true) {
            udp::endpoint sender_endpoint;
            const size_t len = socket.receive_from(net::buffer(buffer), sender_endpoint);
            // ReSharper disable once CppTooWideScopeInitStatement
            const size_t frames = len / player.GetFrameSize();

            if (frames > 0) {
                std::cout << "Received " << len << " bytes. Playing..." << std::endl;
                player.PlayBuffer(buffer.data(), frames, std::chrono::milliseconds(1500));
            }
        }

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

void StartClient(const uint16_t port) {
    try {
        net::io_context io_context;
        udp::socket socket(io_context, udp::v4());
        Recorder recorder(ma_format_u8, 1);

        while (true) {
            std::string server_ip;
            std::cout << "Enter server IP address (and press Enter to record & send): ";
            std::getline(std::cin, server_ip);

            if (server_ip.empty()) continue;

            boost::system::error_code error;
            const auto endpoint = udp::endpoint(net::ip::make_address(server_ip, error), port);
            if (error) {
                std::cout << "Invalid IP: " << error.message() << std::endl;
                continue;
            }

            auto [data, frames] = recorder.Record(65000, std::chrono::milliseconds(1500));
            std::cout << "Recording done. Sending..." << std::endl;

            const size_t size = frames * recorder.GetFrameSize();

            socket.send_to(net::buffer(data.data(), size), endpoint);
            std::cout << "Sent successfully!" << std::endl;
        }

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

int main(const int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <client|server> <port>" << std::endl;
        return 1;
    }

    const std::string mode = argv[1];
    const uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    if (mode == "server") {
        StartServer(port);
    } else if (mode == "client") {
        StartClient(port);
    } else {
        std::cerr << "Unknown mode: " << mode << ". Use 'client' or 'server'." << std::endl;
        return 1;
    }

    return 0;
}