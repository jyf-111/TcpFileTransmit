#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <string>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"

using namespace spdlog;

void TcpServer::run() {
    handleAccept();
    try {
        io.run();
    } catch (asio::system_error& e) {
        error("Error: {}", e.what());
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

std::string TcpServer::handleFileAction(ProtoBuf& protoBuf) {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    std::string result;
    File file(path);

    debug("method: {} path: {} data: {}", ProtoBuf::MethodToString(method),
          path.string(), data);

    switch (method) {
        case ProtoBuf::Method::Get: {
            result = file.QueryDirectory();
            result += "\n";
            break;
        }
        case ProtoBuf::Method::Post: {
            auto data = protoBuf.GetData();
            file.SetFileData(data);
            result = "add_file_OK\n";
            break;
        }
        case ProtoBuf::Method::Delete:
            file.DeleteActualFile();
            result = "delete_file_OK\n";
            break;
        default:
            throw std::runtime_error("unknown method");
    }
    return result;
}

void TcpServer::handleAccept() {
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr =
        std::make_shared<asio::ip::tcp::socket>(io);

    debug("waiting connection");
    acceptor.async_accept(
        *socket_ptr, [this, socket_ptr](const asio::error_code& e) {
            if (e) {
                socket_ptr->shutdown(asio::ip::tcp::socket::shutdown_both);
                socket_ptr->close();

                info("socket close");
                error("connect Error: {}", e.message());
                return;
            } else {
                handleReadWrite(socket_ptr);
                info("connection accepted");
            }
            handleAccept();
        });
}

void TcpServer::handleReadWrite(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    std::shared_ptr<asio::streambuf> streambuf =
        std::make_shared<asio::streambuf>();
    info("new handle read write");

    asio::async_read_until(
        *socket_ptr, *streambuf, '\n',
        [streambuf, socket_ptr, this](const asio::error_code& e, size_t size) {
            if (e) {
                socket_ptr->shutdown(asio::ip::tcp::socket::shutdown_both);
                socket_ptr->close();
                info("socket close");
                error(e.message());
                return;
            }
            ProtoBuf protoBuf;
            std::shared_ptr<std::string> result;
            try {
                std::istream is(streambuf.get());
                is >> protoBuf;
                result =
                    std::make_shared<std::string>(handleFileAction(protoBuf));
            } catch (const std::exception& e) {
                error(e.what());
                result = std::make_shared<std::string>("error_path_or_data\n");
            }

            asio::async_write(*socket_ptr, asio::buffer(*result),
                              [this, socket_ptr, result](
                                  const asio::error_code& e, size_t size) {
                                  if (e) {
                                      socket_ptr->shutdown(
                                          asio::ip::tcp::socket::shutdown_both);
                                      socket_ptr->close();
                                      info("socket close");
                                      error(e.message());
                                      return;
                                  }
                                  debug("send result to client success");
                                  handleReadWrite(socket_ptr);
                              });
        });
}
