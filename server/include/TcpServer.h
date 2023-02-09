#pragma once

#include <asio.hpp>
#include <filesystem>
#include <memory>

#include "ProtoBuf.h"
#include "ThreadPool.h"
#include "asio/error_code.hpp"

class TcpServer {
    asio::io_service io;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acceptor;
    ThreadPool threadPool;

    /**
     * handle File Action
     */
    std::string handleFileAction(ProtoBuf &protoBuf);
    /**
     * handle answer result
     */
    void handleResult(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                     const std::string result);

    /**
     * handle socket
     */
    void handleSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                      const asio::error_code& e);

    /**
     * handle accept
     */
    void handleAccept();

    /**
     * handle read and write
     */
    void handleReadWrite(std::shared_ptr<asio::ip::tcp::socket> socket_ptr);

   public:
    TcpServer(asio::ip::tcp::endpoint, size_t);
    TcpServer() = delete;
    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

    /**
     * @brief Start the server and wait for connections.
     */
    void Process();
};
