#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "Properties.h"
#include "ServerSession.h"

TcpServer::TcpServer() {
    logger = spdlog::get("logger");
    assert(logger != nullptr);
    io = std::make_shared<asio::io_context>();
}

void TcpServer::run() {
    asio::io_context::work work(*io);
    asio::thread_pool threadPool(threads);
    for (std::size_t i = 0; i < threads; ++i) {
        asio::post(threadPool, [self = shared_from_this()] {
            try {
                self->io->run();
            } catch (asio::system_error& e) {
                error(e.what());
            }
        });
    }
    threadPool.join();
}

void TcpServer::init() {
    const auto& value = Properties::readProperties();
    ip = value["ip"].asString();
    port = value["port"].asLargestUInt();
    threads = value["threads"].asLargestUInt();
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
    }
    certificate = value["certificate"].asString();
    privatekey = value["privatekey"].asString();

    assert(!ip.empty());
    assert(port >= 0);
    assert(threads > 0);

    logger->info(
        "ip:{} port:{} threads:{} certificate: "
        "{} privatekey: {}",
        ip, port, threads, certificate, privatekey);

    ssl_context.set_options(asio::ssl::context::default_workarounds |
                            asio::ssl::context::no_sslv2);
    ssl_context.set_verify_mode(asio::ssl::verify_peer |
                                asio::ssl::verify_fail_if_no_peer_cert);
    ssl_context.set_verify_mode(1);
    ssl_context.use_certificate_file(certificate, asio::ssl::context::pem);
    ssl_context.use_private_key_file(privatekey, asio::ssl::context::pem);
}

void TcpServer::handleAccept() {
    auto socketPtr = std::make_shared<ssl_socket>(*io, ssl_context);
    auto serverSession = std::make_shared<ServerSession>(socketPtr, io);

    serverSession->registerSignal();

    acceptor = std::make_unique<asio::ip::tcp::acceptor>(
        *io, asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port),
        true);  // NOTE: SO_REUSEADDR

    logger->info("waiting connection");
    acceptor->async_accept(
        socketPtr->lowest_layer(), [self = shared_from_this(), socketPtr,
                                    serverSession](const asio::error_code& e) {
            self->logger->info("connection accepted");
            self->handleAccept();
            if (e) {
                self->logger->error("async_accept: " + e.message());
            } else {
                socketPtr->async_handshake(
                    asio::ssl::stream_base::server,
                    [self, socketPtr,
                     serverSession](const asio::error_code& e) {
                        self->logger->info("handshake success");
                        serverSession->doWrite();
                        serverSession->doRead();
                    });
            }
        });
}

std::string TcpServer::getIp() const { return ip; }

void TcpServer::setIp(const std::string& ip) { this->ip = ip; }

[[nodiscard]] std::size_t TcpServer::getPort() const { return port; }

void TcpServer::setPort(const std::size_t& port) { this->port = port; }

[[nodiscard]] std::size_t TcpServer::getThreads() const { return threads; }

void TcpServer::setThreads(const std::size_t& threads) {
    this->threads = threads;
}

[[nodiscard]] std::string TcpServer::getCertificate() const {
    return this->certificate;
}

void TcpServer::setCertificate(const std::string& certificate) {
    this->certificate = certificate;
}

[[nodiscard]] std::string TcpServer::getPrivateKey() const {
    return privatekey;
}

void TcpServer::setPrivateKey(const std::string& private_key) {
    this->privatekey = private_key;
}
