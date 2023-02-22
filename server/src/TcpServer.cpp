#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"
#include "WriteSession.h"
#include "asio/post.hpp"

using namespace spdlog;

std::string TcpServer::getIp() const { return ip; }

void TcpServer::setIp(const std::string& ip) { this->ip = ip; }

[[nodiscard]] std::size_t TcpServer::getPort() const { return port; }

void TcpServer::setPort(const std::size_t& port) { this->port = port; }

[[nodiscard]] std::string TcpServer::getLevel() const { return level; }

void TcpServer::setLevel(const std::string& level) {
    this->level = level;
    if (level == "debug") {
        set_level(spdlog::level::debug);
    } else if (level == "info") {
        set_level(spdlog::level::info);
    } else if (level == "warn") {
        set_level(spdlog::level::warn);
    } else if (level == "err") {
        set_level(spdlog::level::err);
    } else if (level == "critical") {
        set_level(spdlog::level::critical);
    } else if (level == "off") {
        set_level(spdlog::level::off);
    }
}

void TcpServer::setFilesplit(const std::size_t& size) {
    this->filesplit = size;
}

[[nodiscard]] std::size_t TcpServer::getFilesplitsize() const {
    return filesplit;
}

[[nodiscard]] std::size_t TcpServer::getThreads() const { return threads; }

void TcpServer::setThreads(const std::size_t& threads) {
    if (threads > 0) {
        this->threads = threads;
    } else {
        this->threads = std::thread::hardware_concurrency();
    }
}

void TcpServer::handleCloseSocket(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    socket_ptr->close();
    info("socket close");
}

void TcpServer::run() {
    asio::thread_pool threadPool(threads);
    for (std::size_t i = 0; i < threads; ++i) {
        asio::post(threadPool, [self = shared_from_this()] {
            try {
                self->io.run();
            } catch (asio::system_error& e) {
                error(e.what());
            }
        });
    }
    threadPool.join();
}

void TcpServer::handleSignal() {
    sig.async_wait([self = shared_from_this()](const std::error_code& e,
                                               int signal_number) {
        switch (signal_number) {
            case SIGINT:
                info("SIGINT received, shutting down");
                self->io.stop();
                break;
            case SIGTERM:
                info("SIGTerm received, shutting down");
                self->io.stop();
                break;
            default:
                info("default {}", e.message());
        }
    });
}

auto TcpServer::handleFileAction(ProtoBuf& protoBuf)
    -> std::variant<std::string, std::vector<std::vector<char>>> {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    File file(path);

    switch (method) {
        case ProtoBuf::Method::Query: {
            return file.QueryDirectory();
        }
        case ProtoBuf::Method::Get: {
            return file.GetFileDataSplited(filesplit);
        }
        case ProtoBuf::Method::Post: {
            file.SetFileData(data);
            auto index = protoBuf.GetIndex();
            auto total = protoBuf.GetTotal();
            if (index < total) {
                return "server saving file : " + std::to_string(index) + "/" +
                       std::to_string(total);
            } else if (index == total) {
                return "server saving file : " + std::to_string(index) + "/" +
                       std::to_string(total) + " OK";
            } else {
                error("index > total");
                return "error: index > total";
            }
        }
        case ProtoBuf::Method::Delete:
            file.DeleteActualFile();
            return "delete file OK";
        default:
            throw std::runtime_error("unknown method");
    }
    return "unknown method";
}

void TcpServer::handleAccept() {
    std::shared_ptr<asio::ip::tcp::socket> socketPtr =
        std::make_shared<asio::ip::tcp::socket>(io);

    std::shared_ptr<WriteSession> writeSession =
        std::make_shared<WriteSession>(socketPtr);

    auto ep = asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
    acceptor = std::move(asio::ip::tcp::acceptor(io, ep));

    debug("waiting connection");
    acceptor.async_accept(*socketPtr,
                          [self = shared_from_this(), socketPtr,
                           writeSession](const asio::error_code& e) {
                              if (e) {
                                  error("async_accept: " + e.message());
                              } else {
                                  self->handleRead(socketPtr, writeSession);
                                  info("connection accepted");
                              }
                              self->handleAccept();
                          });
}

void TcpServer::handleRead(std::shared_ptr<asio::ip::tcp::socket> socketPtr,
                           std::shared_ptr<WriteSession> writeSession) {
    debug("new read");
    auto streambuf = std::make_shared<asio::streambuf>();

    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();

    asio::async_read(
        *socketPtr, *streambuf,
        [peek, streambuf, socketPtr](const asio::system_error& e,
                                     std::size_t size) -> std::size_t {
            if (e.code()) {
                error("async_reading: {}", e.what());
                return 0;
            }
            if (size == sizeof(std::size_t)) {
                std::memcpy(peek.get(), streambuf.get()->data().data(),
                            sizeof(std::size_t));
            }
            if (size > sizeof(std::size_t) &&
                size == *reinterpret_cast<std::size_t*>(peek.get())) {
                return 0;
            } else {
                return 1;
            }
        },
        [streambuf, socketPtr, writeSession, self = shared_from_this()](
            const asio::error_code& e, std::size_t size) {
            if (e) {
                self->handleCloseSocket(socketPtr);
                error("async_read: {}", e.message());
                return;
            }

            debug("read complete");
            self->handleRead(socketPtr, writeSession);

            std::variant<std::string, std::vector<std::vector<char>>> result;

            ProtoBuf recv;
            try {
                std::istream is(streambuf.get());
                is >> recv;
                result = self->handleFileAction(recv);
            } catch (const std::exception& e) {
                error(e.what());
                result = "some error occured";
            }
            if (result.index() == 0) {
                const auto& str = std::get<std::string>(result);

                ProtoBuf send{ProtoBuf::Method::Post, recv.GetPath(),
                              std::vector<char>(str.begin(), str.end())};
                if (recv.GetMethod() == ProtoBuf::Method::Query) {
                    send.SetIsDir(true);
                }
                writeSession->enqueue(send);
            } else {
                const auto& vec =
                    std::get<std::vector<std::vector<char>>>(result);
                const auto& len = vec.size();
                for (std::size_t i = 0; i < len; ++i) {
                    ProtoBuf ret{ProtoBuf::Method::Post, recv.GetPath(),
                                 vec.at(i)};
                    ret.SetIsFile(true);
                    ret.SetIndex(i);
                    ret.SetTotal(len - 1);
                    writeSession->enqueue(ret);
                }
            }
            writeSession->doWrite();
        });
}
