#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"
#include "asio/error_code.hpp"

TcpServer::TcpServer(asio::ip::tcp::endpoint ep, size_t thread_pool_size)
    : ep(std::move(ep)),
      acceptor(io, std::move(ep)),
      threadPool(thread_pool_size) {}

void TcpServer::Process() {
    using namespace spdlog;
    set_level(spdlog::level::debug);
    while (true) {
        debug("Waiting for connection...");
        std::shared_ptr<asio::ip::tcp::socket> socket_ptr(
            new asio::ip::tcp::socket(io));

        acceptor.accept(*socket_ptr.get());
        debug("Connection accepted");

        threadPool.enqueue([socket_ptr]() {
            while (true) {
                try {
                    // TODO: read some data from client
                    File file;
                    std::array<char, BUF_SIZE> buf;

                    socket_ptr->read_some(asio::buffer(buf));
                    debug("Data received {}", strlen(buf.data()));

                    ProtoBuf pb;
                    pb.SetProtoBuf(buf);

                    auto method = pb.GetMethod();
                    auto path = pb.GetPath();
                    debug("method: {}", ProtoBuf::MethodToString(method));
                    debug("path: {}", path.string());

                    file.SetFilePath(path);

                    std::string result;
                    switch (method) {
                        case ProtoBuf::Method::Get: {
                            result = file.QueryDirectory();
                            break;
                        }
                        case ProtoBuf::Method::Post: {
                            auto data =
                                pb.GetData<std::array<char, BUF_SIZE>>();
                            debug("data: {}", data.data());
                            file.SetFileData(
                                std::string(data.data(), strlen(data.data())));
                            result = "add file OK";
                            break;
                        }
                        case ProtoBuf::Method::Delete:
                            file.DeleteActualFile();
                            result = "delete file OK";
                            break;
                        default:
                            break;
                    }
                    socket_ptr->write_some(asio::buffer(result));
                    debug("send result to client");
                    debug("data {}", result);

                } catch (asio::system_error &e) {
                    if (e.code() == asio::error::eof ||
                        e.code() == asio::error::connection_reset) {
                        socket_ptr->close();
                        debug("Connection closed");
                        break;
                    } else {
                        error("Error: {}", e.what());
                        std::string tmp(e.what());
                        socket_ptr->write_some(asio::buffer(tmp));
                        debug("send error message to client");
                        debug("Connection closed");
                    }
                }
            }
        });
    }

}
