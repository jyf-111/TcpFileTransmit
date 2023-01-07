#include "TcpServer.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"

TcpServer::TcpServer(asio::ip::tcp::endpoint ep, size_t thread_pool_size)
    : ep(std::move(ep)),
      acceptor(io, std::move(ep)),
      threadPool(thread_pool_size) {
    using namespace spdlog;
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

                    // NOTE: init file
                    File file;

                    // NOTE: init buffer
                    std::array<char, BUF_SIZE> buf;

                    // NOTE: read
                    socket_ptr->read_some(asio::buffer(buf));
                    info("Data received {}", strlen(buf.data()));

                    // NOTE: protoBuf
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
                            // NOTE: Get
                            result = file.QueryDirectory();
                            break;
                        }
                        case ProtoBuf::Method::Post: {
                            // NOTE: Post
                            auto data = pb.GetData<std::array<char, BUF_SIZE>>();
                            debug("data: {}", data.data());
                            file.SetFileData(
                                std::string(data.data(), strlen(data.data())));
                            result = "add file OK";
                            break;
                        }
                        case ProtoBuf::Method::Delete:
                            // NOTE: Delete
                            file.DeleteActualFile();
                            result = "delete file OK";
                            break;
                        default:
                            break;
                    }
                    // NOTE: send result to client
                    socket_ptr->write_some(asio::buffer(result));
					info("send result to client");
					info("data {}", result);

                } catch (asio::system_error &e) {
                    if (e.code() == asio::error::eof ||
                        e.code() == asio::error::connection_reset) {
                        socket_ptr->close();
                        info("Connection closed");
                        break;
                    } else {
                        // NOTE: send result to client
                        error("Error: {}", e.what());
                        std::string tmp(e.what());
                        socket_ptr->write_some(asio::buffer(tmp));
                        info("send error message to client");
                        info("Connection closed");
                    }
                }
            }
        });
    }
}
