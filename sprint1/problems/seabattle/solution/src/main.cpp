#ifdef WIN32
#include <sdkddkver.h>
#endif

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    /**
     * @param socket server socket
     * @param my_initiative response for marking player turn. Changes when turn comes up to another player
     */

    void StartGame(tcp::socket& socket, bool my_initiative) {
        PrintFields();

        while (!IsGameEnded()) {
            if (my_initiative) {
                std::cout << "Major: Charge is ready, waiting for coordinates, Captain!" << std::endl << "Captain: Shoot ";
                std::string move;

                std::pair<int, int> parsed_move;
                while (true) {
                    std::string move_str;
                    std::getline(std::cin, move_str);
                    if (auto p = ParseMove(move_str)) {
                        parsed_move = *p;
                        if (other_field_(parsed_move.first, parsed_move.second) == SeabattleField::State::UNKNOWN) {
                            break;
                        }
                    }
                }

                SendMove(socket, parsed_move);

                switch (const auto result = ReadResult(socket); *result) {
                    case SeabattleField::ShotResult::MISS: {
                        other_field_.MarkMiss(parsed_move.first, parsed_move.second);
                        std::cout << "Major: Crap, shell missed" << std::endl;
                        my_initiative = false;
                        break;
                    }

                    case SeabattleField::ShotResult::HIT: {
                        other_field_.MarkHit(parsed_move.first, parsed_move.second);
                        std::cout << "Major: Got those RATS! We hit 'em!" << std::endl;
                        break;
                    }

                    case SeabattleField::ShotResult::KILL: {
                        other_field_.MarkKill(parsed_move.first, parsed_move.second);
                        std::cout << "Major: This one is sinking, Captain!" << std::endl;
                        break;
                    }
                }
                PrintFields();

            } else {
                std::cout << "Major: Weapons are on cooldown!" << std::endl;

                const auto move = ReadMove(socket);
                if (!move) {
                    std::cerr << "Opponent disconnected unexpectedly." << std::endl;
                    break;
                }
                const SeabattleField::ShotResult res = my_field_.Shoot(move->first, move->second);

                SendResult(socket, res);
                PrintFields();

                if (res == SeabattleField::ShotResult::MISS)
                    my_initiative = true;
            }
        }
        if (my_field_.IsLoser()) {
            std::cout << "Major: All ships are gone, we lost this..." << std::endl;
        } else {
            std::cout << "Major: They are running away! We got this!" << std::endl;
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int y = sv[0] - 'A';
        int x = sv[1] - '1';

        if (x < 0 || x >= SeabattleField::field_size) return std::nullopt;
        if (y < 0 || y >= SeabattleField::field_size) return std::nullopt;

        return {{x, y}};
    }

    static std::string MoveToString(const std::pair<int, int> &move) {
        char buff[] = {static_cast<char>(move.second + 'A'), static_cast<char>(move.first + '1')};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    void SendMove(tcp::socket& socket, const std::pair<int, int>& move) {
        const std::string move_str = MoveToString(move);
        WriteExact(socket, move_str);
    }

    std::optional<std::pair<int, int>> ReadMove(tcp::socket& socket) {
        const auto move_str = ReadExact<2>(socket);
        if (!move_str) return std::nullopt;
        return ParseMove(*move_str);
    }

    static void SendResult(tcp::socket& socket, SeabattleField::ShotResult result) {
        char res = static_cast<char>(result);
        WriteExact(socket, {&res, 1});
    }

    static std::optional<SeabattleField::ShotResult> ReadResult(tcp::socket& socket) {
        const auto res_str = ReadExact<1>(socket);
        if (!res_str) return std::nullopt;
        int res_int = (*res_str)[0];
        if (res_int < 0 || res_int > 2) return std::nullopt;
        return static_cast<SeabattleField::ShotResult>(res_int);
    }

    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, {tcp::v4(), port});
    tcp::socket socket(io_context);
    acceptor.accept(socket);
    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);
    net::io_context io_context;
    boost::system::error_code error;
    const tcp::endpoint endpoint(net::ip::make_address(ip_str, error), port);
    if (error) throw std::runtime_error(error.message());

    tcp::socket socket(io_context);
    socket.connect(endpoint, error);
    if (error) throw std::runtime_error(error.message());

    agent.StartGame(socket, true);
};

int main(const int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        try {
            StartClient(fieldL, argv[2], std::stoi(argv[3]));
        } catch (std::exception &e) {
            std::cerr << e.what();
        }
    }
}
