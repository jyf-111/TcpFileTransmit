#pragma once

#include <asio.hpp>
#include <filesystem>
#include <memory>

#include "ProtoBuf.h"

class TcpServer {
    asio::io_service io;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acceptor;

    /**
     * handle File Action
     */
    std::string handleFileAction(ProtoBuf &protoBuf);

    /**
     * handle socket
     */
    void handleSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr);


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
     * handle accept
     */
    void handleAccept();
};
