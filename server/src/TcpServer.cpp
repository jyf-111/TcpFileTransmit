#include "TcpServer.h"

#include <spdlog/spdlog.h>
#include <vcruntime.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"
#include "asio/streambuf.hpp"

using std::make_shared;

using namespace spdlog;

void TcpServer::handleCloseSocket(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    socket_ptr->shutdown(asio::ip::tcp::socket::shutdown_both);
    socket_ptr->close();
    info("socket close");
}

void TcpServer::run() {
    try {
        io.run();
    } catch (asio::system_error& e) {
        error("Run Error: {}", e.what());
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

void TcpServer::readProperties() {
    std::string ip = "127.0.0.1";
    size_t port = 8000;
    std::string level = "info";
    try {
        Properties properties;
        auto value = properties.readProperties();
        ip = value["ip"].asString();
        port = value["port"].asUInt();
        level = value["log"].asString();
    } catch (std::exception& e) {
        warn("{}", e.what());
    }

    auto ep = asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
    acceptor = std::move(asio::ip::tcp::acceptor(io, ep));

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

std::variant<std::string, std::vector<std::vector<char>>>
TcpServer::handleFileAction(ProtoBuf& protoBuf) {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    File file(path);

    debug("method: {} path: {} ", ProtoBuf::MethodToString(method),
          path.string());

    switch (method) {
        case ProtoBuf::Method::Query: {
            return file.QueryDirectory();
        }
        case ProtoBuf::Method::Get: {
            return file.GetFileDataSplited(65536 * 3);
        }
        case ProtoBuf::Method::Post: {
            file.SetFileData(data);
            return "add file ok";
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

    debug("waiting connection");
    acceptor.async_accept(*socket_ptr,
                          [this, socket_ptr](const asio::error_code& e) {
                              if (e) {
                                  handleCloseSocket(socket_ptr);
                                  error("async_accept: " + e.message());
                              } else {
                                  handleRead(socket_ptr);
                                  info("connection accepted");
                              }
                              handleAccept();
                          });
}

void TcpServer::handleRead(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    auto streambuf = std::make_shared<asio::streambuf>();
    info("new handle read write");

    auto peek = make_shared<std::array<char, sizeof(size_t)>>();
    asio::async_read(
        *socket_ptr, *streambuf,
        [peek, streambuf, this, socket_ptr](const asio::system_error& e,
                                            size_t size) -> size_t {
            if (e.code()) {
                error("async_reading: {}", e.what());
                return 0;
            }
            if (size == sizeof(size_t)) {
                std::memcpy(peek.get(), streambuf.get()->data().data(),
                            sizeof(size_t));
            }
            if (size > sizeof(size_t) &&
                size == *reinterpret_cast<size_t*>(peek.get())) {
                return 0;
            } else {
                return 1;
            }
        },
        [streambuf, socket_ptr, this](const asio::error_code& e, size_t size) {
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
                ret.SetFlag(true);
                handleWrite(socket_ptr, ret);

            } else {
                const auto& vec =
                    std::get<std::vector<std::vector<char>>>(result);
                for (const auto& v : vec) {
                    ProtoBuf ret{ProtoBuf::Method::Post, protoBuf.GetPath(), v};
                    handleWrite(socket_ptr, ret);
                }
            }
        });
}
void TcpServer::handleWrite(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                            const ProtoBuf& protobuf) {
    auto buf = std::make_shared<asio::streambuf>();
    std::ostream os(buf.get());
    os << protobuf;

    asio::async_write(
        *socket_ptr, *buf,
        [this, socket_ptr](const asio::error_code& e, size_t size) {
            if (e) {
                handleCloseSocket(socket_ptr);
                error("async_write: {}", e.message());
                return;
            }
            debug("send result to client success");
        });
}
