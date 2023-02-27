#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <csignal>
#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

#include "ProtoBuf.h"
#include "WriteSession.h"

class TcpServer : public std::enable_shared_from_this<TcpServer> {
    std::string ip = "127.0.0.1";
    std::size_t port = 8000;
    std::size_t filesplit = 65536 * 4;
    std::size_t threads;
    std::string certificate;
    std::string private_key;

    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<asio::io_context> io;
    std::shared_ptr<asio::io_context::strand> fileWriteStrand;
    std::shared_ptr<asio::signal_set> sig;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;

    void handleCloseSocket(std::shared_ptr<ssl_socket>);
    auto handleFileAction(ProtoBuf &)
        -> std::variant<std::string, std::vector<std::vector<char>>>;
    void handleRead(std::shared_ptr<ssl_socket>, std::shared_ptr<WriteSession>);
    void handleSignal(std::weak_ptr<ssl_socket>);

   public:
    TcpServer();
    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;

    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &);
    [[nodiscard]] std::size_t getPort() const;
    void setPort(const std::size_t &);
    [[nodiscard]] std::size_t getFilesplitsize() const;
    void setFilesplit(const std::size_t &);
    [[nodiscard]] std::size_t getThreads() const;
    void setThreads(const std::size_t &);
    [[nodiscard]] std::string getCertificate() const;
    void setCertificate(const std::string &);
    [[nodiscard]] std::string getPrivateKey() const;
    void setPrivateKey(const std::string &);

    void handleAccept();
    void run();
};
