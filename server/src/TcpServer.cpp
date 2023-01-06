#include "TcpServer.h"

#include <spdlog/spdlog.h>
#include <windef.h>

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"

using namespace spdlog;

TcpServer::TcpServer(asio::ip::tcp::endpoint ep, size_t thread_pool_size)
    : ep(std::move(ep)),
      acceptor(io, std::move(ep)),
      threadPool(thread_pool_size) {
	// NOTE: set_level
    set_level(spdlog::level::debug);
    while (true) {
        info("Waiting for connection...");
        std::shared_ptr<asio::ip::tcp::socket> socket_ptr(
            new asio::ip::tcp::socket(io));
        acceptor.accept(*socket_ptr.get());

        info("Connection accepted");

        // NOTE: push task to thread pool
        threadPool.enqueue([socket_ptr]() {
            while (true) {
                try {
                    // TODO: read some data from client
                    // NOTE: std::string is not available in asio::buffer
                    // when recvive, see it in:
                    // https://www.codenong.com/4068249/

                    // init file
                    File file("result.txt");

                    // init buffer
                    std::array<char, SIZE> buf;

                    // read
                    socket_ptr->read_some(asio::buffer(buf));
                    info("Data received {}", strlen(buf.data()));

                    // protoBuf
                    ProtoBuf pb;
                    pb.SetProtoBuf(buf);

                    // FIXME: data can only std::array
                    auto method = pb.GetMethod();
                    auto path = pb.GetPath();
                    auto data = pb.GetData<std::array<char, SIZE>>();

                    debug("method: {}", ProtoBuf::MethodToString(method));
                    debug("path: {}", path.string());
                    debug("data: {}", data.data());

                    // write file
                    file.SetFilePath(path);

                    file.SetFileData(
                        std::string(data.data(), strlen(data.data())));

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
        });
    }
}
