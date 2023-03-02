#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <csignal>
#include <memory>
#include <variant>
#include <vector>

#include "ProtoBuf.h"

class TcpServer : public std::enable_shared_from_this<TcpServer> {
    std::shared_ptr<spdlog::logger> logger;

    std::string ip;
    std::size_t port;
    std::size_t threads;
    std::string certificate;
    std::string privatekey;

    asio::ssl::context ssl_context{asio::ssl::context::tls};
    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<asio::io_context> io;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;

    auto handleFileAction(ProtoBuf &)
        -> std::variant<std::string, std::vector<std::vector<char>>>;

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
    [[nodiscard]] std::size_t getThreads() const;
    void setThreads(const std::size_t &);
    [[nodiscard]] std::string getCertificate() const;
    void setCertificate(const std::string &);
    [[nodiscard]] std::string getPrivateKey() const;
    void setPrivateKey(const std::string &);

    void init();
    void handleAccept();
    void run();
};
