#pragma once

#include <asio.hpp>
#include <csignal>
#include <filesystem>
#include <memory>

#include "ProtoBuf.h"

class TcpServer {
    asio::io_service io;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::acceptor acceptor;
    asio::signal_set sig{io, SIGINT, SIGTERM};

    /**
     * handle File Action
     */
    std::string handleFileAction(ProtoBuf &protoBuf);

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
     * handle signal
     */
    void handleSignal();
    /**
     * handle accept
     */
    void handleAccept();

    /**
     * run
     */
    void run();
};
