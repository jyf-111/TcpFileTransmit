#pragma once

#include <asio.hpp>
#include <csignal>
#include <filesystem>
#include <memory>

#include "ProtoBuf.h"

class TcpServer {
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor{io};
    asio::signal_set sig{io, SIGINT, SIGTERM};

    /**
     * handle close socket
     */
    void handleCloseSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr);

    /**
     * handle File Action
     */
    std::string handleFileAction(ProtoBuf &protoBuf);

    /**
     * handle read and write
     */
    void handleReadWrite(std::shared_ptr<asio::ip::tcp::socket> socket_ptr);

   public:
    TcpServer() = default;
    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

    /**
     * read properties
     */
    void readProperties();
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
