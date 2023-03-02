#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <queue>

#include "ProtoBuf.h"

namespace app {
class TcpClient;
}

class ClientSession : public std::enable_shared_from_this<ClientSession> {
    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<ssl_socket> socketPtr;
    std::queue<ProtoBuf> writeQueue;
    std::shared_ptr<asio::steady_timer> timer;
    std::shared_ptr<asio::steady_timer> queryTimer;
    std::shared_ptr<spdlog::logger> logger;
    std::unique_ptr<asio::io_context::strand> fileWriteStrand;
    std::shared_ptr<app::TcpClient> client;
    std::mutex mtx;

   public:
    ClientSession(std::shared_ptr<ssl_socket> socketPtr,
                  std::shared_ptr<asio::io_context> io);

    void initClient(std::weak_ptr<app::TcpClient> client);
    void enqueue(const ProtoBuf &buf);
    void doRead();
    void doWrite();
    void registerQuery();
};
