#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"

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

void TcpServer::handleCloseSocket(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    socket_ptr->shutdown(asio::ip::tcp::socket::shutdown_both);
    socket_ptr->close();
    info("socket close");
}

void TcpServer::run() {
    auto run = ([this]() {
        try {
            io.run();
        } catch (asio::system_error& e) {
            error("Run Error: {}", e.what());
        }
    });
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back(run);
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

void TcpServer::handleSignal() {
    sig.async_wait([this](const std::error_code& e, int signal_number) {
        switch (signal_number) {
            case SIGINT:
                info("SIGINT received, shutting down");
                io.stop();
                break;
            case SIGTERM:
                info("SIGTerm received, shutting down");
                io.stop();
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
            debug("Get {}", path.string());
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
            return "delete_file_OK";
        default:
            throw std::runtime_error("unknown method");
    }
    return "unknown method";
}

void TcpServer::handleAccept() {
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr =
        std::make_shared<asio::ip::tcp::socket>(io);

    auto ep = asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
    acceptor = std::move(asio::ip::tcp::acceptor(io, ep));

    debug("waiting connection");
    acceptor.async_accept(*socket_ptr,
                          [this, socket_ptr](const asio::error_code& e) {
                              if (e) {
                                  error("async_accept: " + e.message());
                              } else {
                                  handleRead(socket_ptr);
                                  info("connection accepted");
                              }
                              handleAccept();
                          });
}

void TcpServer::handleRead(std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    debug("new read");
    auto streambuf = std::make_shared<asio::streambuf>();

    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();
    asio::async_read(
        *socket_ptr, *streambuf,
        [peek, streambuf, this, socket_ptr](const asio::system_error& e,
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
        [streambuf, socket_ptr, this](const asio::error_code& e,
                                      std::size_t size) {
            if (e) {
                handleCloseSocket(socket_ptr);
                error("async_read: {}", e.message());
                return;
            }

            debug("read complete");

            handleRead(socket_ptr);

            std::variant<std::string, std::vector<std::vector<char>>> result;

            ProtoBuf protoBuf;
            try {
                std::istream is(streambuf.get());
                is >> protoBuf;
                result = handleFileAction(protoBuf);
            } catch (const std::exception& e) {
                error(e.what());
                result = "some error occured";
            }
            if (result.index() == 0) {
                const auto& str = std::get<std::string>(result);

                ProtoBuf ret{ProtoBuf::Method::Post, protoBuf.GetPath(),
                             std::vector<char>(str.begin(), str.end())};
                handleWrite(socket_ptr, ret);

            } else {
                const auto& vec =
                    std::get<std::vector<std::vector<char>>>(result);
                const auto& len = vec.size();
                for (std::size_t i = 0; i < len; i++) {
                    ProtoBuf ret{ProtoBuf::Method::Post, protoBuf.GetPath(),
                                 vec.at(i)};
                    ret.SetIsFile(true);
                    ret.SetIndex(i);
                    ret.SetTotal(len - 1);
                    handleWrite(socket_ptr, ret);
                }
            }
        });
}
void TcpServer::handleWrite(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                            const ProtoBuf& protobuf) {
    debug("new write");
    auto buf = std::make_shared<asio::streambuf>();
    std::ostream os(buf.get());
    os << protobuf;

    writeStrand.post([this, socket_ptr, buf]() {
        asio::async_write(
            *socket_ptr, *buf,
            [this, socket_ptr](const asio::error_code& e, std::size_t size) {
                if (e) {
                    handleCloseSocket(socket_ptr);
                    error("async_write: {}", e.message());
                    return;
                }
                debug("send result to client success");
            });
    });
}
