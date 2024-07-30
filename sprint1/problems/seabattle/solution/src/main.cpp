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

using ShotResult = SeabattleField::ShotResult;

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

std::ostream& operator<<(std::ostream& output, const ShotResult& result) {
    switch (result) {
        case ShotResult::MISS:
            std::cout << "Miss!"sv;
            break;

        case ShotResult::HIT:
            std::cout << "Hit!"sv;
            break;

        case ShotResult::KILL:
            std::cout << "Kill!"sv;
            break;

        default:
            break;
    }
    return output;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while (!IsGameEnded()) {
            PrintFieldPair(my_field_, other_field_);
            if (my_initiative) {
                std::cout << "Your turn: "sv;

                std::string move;
                std::cin >> move;

                auto parsed_move = ParseMove(move);
                if (!parsed_move) {
                    std::cout << "Invalid position! Try again"sv << std::endl;
                    continue;
                }

                SendMove(socket, move);

                ShotResult result = static_cast<ShotResult>(ReadResult(socket));

                std::cout << result << std::endl;

                UpdateOpponentField(parsed_move.value(), result);
                my_initiative = result == ShotResult::MISS ? false : true;
            } else {
                std::cout << "Waiting for the opponent's move..."sv << std::endl;
                std::string move = ReadMove(socket);
                std::cout << "Shot to "sv << move << std::endl;

                auto parsed_move = ParseMove(move);
                ShotResult shot_res;
                shot_res = my_field_.Shoot(parsed_move.value().second, parsed_move.value().first);
                SendResult(socket, shot_res);
                if (shot_res == ShotResult::MISS) my_initiative = true;
            }
        }
        CheerWinner();
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(static_cast<char>(move.first) + 'A'), 
                       static_cast<char>(static_cast<char>(move.second) + '1')};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    std::string ReadMove(tcp::socket& socket) {
        auto move = ReadExact<2>(socket);
        if (!move) {
            throw std::runtime_error("Error reading move"s);
        }
        return *move;
    }

    char ReadResult(tcp::socket& socket) {
        auto move = ReadExact<1>(socket);
        if (!move) {
            throw std::runtime_error("Error reading shot result"s);
        }

        return move->front();
    }

    void SendResult(tcp::socket& socket, SeabattleField::ShotResult shot_res) {
        std::string coded_shot_res{static_cast<char>(shot_res)};
        if (!WriteExact(socket, coded_shot_res)) {
            throw std::runtime_error("Error sending shot result"s);;
        }
    }

    void SendMove(tcp::socket& socket, std::string_view move) {
        if (!WriteExact(socket, move)) {
            throw std::runtime_error("Error sending move"s);;
        }
    }

    void UpdateOpponentField(std::pair<int, int>& move, SeabattleField::ShotResult res) {
        switch (res) {
            case ShotResult::MISS:
                other_field_.MarkMiss(move.second, move.first);
                break;

            case ShotResult::HIT:
                other_field_.MarkHit(move.second, move.first);
                break;

            case ShotResult::KILL:
                other_field_.MarkKill(move.second, move.first);
                break;

            default:
                break;
        }
    }

    void CheerWinner() {
        if (my_field_.IsLoser() && !other_field_.IsLoser()) {
            std::cout << "Unluck. Don't worry, you will win next time"sv << std::endl;
        } else if (!my_field_.IsLoser() && other_field_.IsLoser()) {
            std::cout << "Good job! You are the winner"sv << std::endl;
        } else {
            std::cout << "Hmmmmm. The situation is unusual. Better to notify programmists because they did something wrong"sv << std::endl;
        }
    }

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);
    
    net::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io_context};
    acceptor.accept(socket, ec);
    if (ec) {
        std::cout << "Can't accept connection"sv << std::endl;
        throw ec;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);
    if (ec) {
        std::cout << "Wrong IP format"sv << std::endl;
        return 1;
    }

    net::io_context io_context;
    tcp::socket socket{io_context};
    socket.connect(endpoint, ec);
    if (ec) {
        std::cout << "Can't connect to server"sv << std::endl;
    }

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}
