
#include "TcpServer.h"

#include <spdlog/spdlog.h>
#include <windef.h>

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

#include "ProtoBuf.h"

using namespace spdlog;

TcpServer::TcpServer(asio::ip::tcp::endpoint ep)
    : ep(std::move(ep)), acceptor(io, std::move(ep)) {
    while (true) {
        info("Waiting for connection...");
        std::shared_ptr<asio::ip::tcp::socket> socket_ptr(
            new asio::ip::tcp::socket(io));
        acceptor.accept(*socket_ptr.get());

        info("Connection accepted");
        std::thread([socket_ptr]() {
            while (true) {
                try {
                    // TODO: read some data from client
                    std::array<char, SIZE> buf;
                    socket_ptr->read_some(asio::buffer(buf));
                    ProtoBuf pb;
					pb.SetProtoBuf(buf);
                    std::cout << pb.GetData().data() << std::endl;
                } catch (asio::system_error &e) {
                    if (e.code() == asio::error::eof ||
                        e.code() == asio::error::connection_reset) {
                        info("Connection closed");
                        break;
                    } else {
                        error("Error: {}", e.what());
                        break;
                    }
                }
            }
        }).detach();
    }
}
