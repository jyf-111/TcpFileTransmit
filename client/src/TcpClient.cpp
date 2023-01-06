#include "TcpClient.h"

#include "ProtoBuf.h"
#include <iostream>

using namespace spdlog;

TcpClient::TcpClient(std::string ip, size_t port)
    : ep(asio::ip::address::from_string(ip), port), sock(io) {
    // NOTE: set_level
    set_level(spdlog::level::debug);
    sock.connect(ep);
    info("Connect success");

    try {
        // TODO: protoBuf
        ProtoBuf pb;
        pb.SetMethod(ProtoBuf::Method::Get);
        pb.SetPath(".");

        debug("path: {}", pb.GetPath().string());
        pb.SetData(std::array<char, SIZE>{'1', '2', '3'});

        sock.write_some(asio::buffer(pb.GetProtoBuf<std::array<char, SIZE>>()));
        info("Send success");

        std::array<char, SIZE> buf;
        size_t size = sock.read_some(asio::buffer(buf));
        info("Receive success {} byte", size);
        std::cout << buf.data() << std::endl;
    } catch (std::exception &e) {
        error("Error: {}", e.what());
    }
}
