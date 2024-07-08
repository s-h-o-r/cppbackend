/*
 #include "audio.h"

#include <boost/asio.hpp>
#include <array>
#include <iostream>

namespace net = boost::asio;
using net::ip::udp;
using namespace std::literals;

void StartServer(uint16_t port) {
    boost::system::error_code ec;

    net::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

    Player player(ma_format_u8, 1);

    for (;;) {
        udp::endpoint remote_endpoint;
        std::vector<char> recv_buf;

        size_t datagram_size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

        player.PlayBuffer(recv_buf.data(), datagram_size / player.GetFrameSize(), 1.5s);
    }
}

void StartClient(uint16_t port) {
    boost::system::error_code ec;
    net::io_context io_context;

    udp::socket socket(io_context, udp::v4());

    auto endpoint = udp::endpoint(net::ip::make_address("127.0.0.1"sv, ec), port);
    if (ec) {
        std::cout << "Unable to make IP address"sv << std::endl;
    }

    Recorder recoder(ma_format_u8, 1);
    
    auto data = recoder.Record(65000, 1.5s);

    std::cout << "Recording done" << std::endl;

    socket.send_to(net::buffer(data.data, data.frames * recoder.GetFrameSize()), endpoint);

    std::cout << "Recording has been sent successfully" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: "sv << argv[0] << " <\"client\" or \"server\"> <server port>"sv << std::endl;
        return 1;
    }

    if (argv[1] == "client"sv) {
        StartClient(*argv[2]);
    } else if (argv[1] == "server"sv) {
        StartServer(*argv[2]);
    } else {
        std::cout << "Unknown role. Enter \"client\" or \"server\""sv << std::endl;
        return 1;
    }
}
 */

#include "audio.h"
#include <iostream>

using namespace std::literals;

int main(int argc, char** argv) {
    Recorder recorder(ma_format_u8, 1);
    Player player(ma_format_u8, 1);

    while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
        std::cout << "Playing done" << std::endl;
    }

    return 0;
}
