#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <queue>
#include <variant>

#include "ProtoBuf.h"
#include "spdlog/logger.h"

using namespace spdlog;

class ServerSession : public std::enable_shared_from_this<ServerSession> {
    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<asio::io_context> io;
    std::shared_ptr<ssl_socket> socketPtr;
    std::queue<ProtoBuf> writeQueue;
    std::shared_ptr<asio::steady_timer> timer;
    std::shared_ptr<asio::signal_set> sig;
    std::shared_ptr<spdlog::logger> logger;
    std::mutex mtx;
    std::shared_ptr<asio::io_context::strand> fileWriteStrand;
    std::size_t filesplit;

   public:
    ServerSession(std::shared_ptr<ssl_socket> socketPtr,
                  std::shared_ptr<asio::io_context> io);
    void registerSignal();
    void enqueue(const ProtoBuf& buf);

    void handleCloseSocket();
    auto handleFileAction(ProtoBuf& protoBuf)
        -> std::variant<std::string, std::vector<std::vector<char>>>;

    void doWrite();
    void doRead();
};
