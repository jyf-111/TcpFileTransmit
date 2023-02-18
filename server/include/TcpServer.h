#pragma once

#include <asio.hpp>
#include <csignal>
#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

#include "ProtoBuf.h"
#include "asio/io_context.hpp"

class TcpServer {
    asio::io_context io;
    asio::ip::tcp::acceptor acceptor{io};
    asio::io_context::strand writeStrand{io};
    asio::signal_set sig{io, SIGINT, SIGTERM};

    std::string ip = "127.0.0.1";
    std::size_t port = 8000;
    std::string level = "info";
    std::size_t filesplit = 65536 * 4;

    /**
     * handle close socket
     */
    void handleCloseSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr);

    /**
     * handle File Action
     */
    auto handleFileAction(ProtoBuf &protoBuf)
        -> std::variant<std::string, std::vector<std::vector<char>>>;

    /**
     * handle read
     */
    void handleRead(std::shared_ptr<asio::ip::tcp::socket> socket_ptr);

    /**
     * handle write
     */

    void handleWrite(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                     const ProtoBuf &);

   public:
    TcpServer() = default;
    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &);
    [[nodiscard]] std::size_t getPort() const;
    void setPort(const size_t &);
    [[nodiscard]] std::string getLevel() const;
    void setLevel(const std::string &);
    [[nodiscard]] std::size_t getFilesplitsize() const;
    void setFilesplit(const std::size_t &);

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
