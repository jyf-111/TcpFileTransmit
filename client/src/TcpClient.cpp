#include "TcpClient.h"

#include <filesystem>
#include <stdexcept>
#include <thread>
#include <vector>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"
#include "asio/socket_base.hpp"
#include "spdlog/spdlog.h"

void app::TcpClient::readProperties() {
    std::string ip = "127.0.0.1";
    size_t port = 8000;
    std::string level = "info";
    try {
        Properties properties;
        auto value = properties.readProperties();
        ip = value["ip"].asString();
        port = value["port"].asUInt();
        level = value["log"].asString();
    } catch (std::exception &e) {
        warn("{}", e.what());
    }

    ep = asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
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
    } else {
        set_level(spdlog::level::info);
    }
}

void app::TcpClient::handleResult(std::string &result) {
    result.clear();
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    std::string tmp;
    tmp += "[";
    tmp += std::to_string(now->tm_hour);
    tmp += ":";
    tmp += std::to_string(now->tm_min);
    tmp += ":";
    tmp += std::to_string(now->tm_sec);
    tmp += "]";
    tmp.append(result);
    tmp.append("\n");
    result = tmp;
}

void app::TcpClient::handleRead() {
    if (!connectFlag) throw std::runtime_error("not connected");
    auto resultBuf = std::make_shared<asio::streambuf>();

    auto streambuf = std::make_shared<asio::streambuf>();
    auto peek = std::make_shared<std::array<char, sizeof(size_t)>>();

    asio::async_read(
        tcpSocket, *streambuf,
        [peek, streambuf, this](const asio::system_error &e,
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
                size == *reinterpret_cast<size_t *>(peek.get())) {
                return 0;
            } else {
                return 1;
            }
        },
        [streambuf, this](const asio::error_code &e, size_t size) {
            if (e) {
                disconnect();
                error("async_read: {}", e.message());
                return;
            }
            debug("read complete");

            handleRead();

            ProtoBuf protoBuf;
            std::istream is(streambuf.get());
            is >> protoBuf;

            handleResult(result);
            const bool flag = protoBuf.GetFlag();
            if (flag) {
                const auto &data = protoBuf.GetData();
                result += std::string(data.begin(), data.end());
            } else {
                File file(protoBuf.GetPath());
                file.SetFileData(protoBuf.GetData());
            }
        });
}

void app::TcpClient::handleWrite(const ProtoBuf &protobuf) {
    if (!connectFlag) {
        result = "not connected";
        error("{}", result);
        return;
    }

    debug("new read and write");
    auto buf = std::make_shared<asio::streambuf>();
    auto os = std::make_shared<std::ostream>(buf.get());
    *os << protobuf;
    debug("Send: {} {} ", ProtoBuf::MethodToString(protobuf.GetMethod()),
          protobuf.GetPath().string());

    // NOTE: async_write
    asio::async_write(tcpSocket, *buf.get(),
                      [this](const asio::error_code &e, size_t size) {
                          if (e) {
                              error("{}", e.message());
                              return;
                          }
                          debug("Send success");
                      });
}

void app::TcpClient::handleQuery(const std::filesystem::path &path) {
    handleWrite(
        {ProtoBuf::Method::Query, path, std::vector<char>{'n', 'u', 'l', 'l'}});
}

void app::TcpClient::handleGet(const std::filesystem::path &path) {
    handleWrite(
        {ProtoBuf::Method::Get, path, std::vector<char>{'n', 'u', 'l', 'l'}});
}

void app::TcpClient::handlePost(const std::filesystem::path &path,
                                const std::vector<char> &data) {
    handleWrite({ProtoBuf::Method::Post, path, data});
}

void app::TcpClient::handlePost(const std::filesystem::path &path,
                                const std::vector<std::vector<char>> &data) {
    const auto lenth = data.size();
    for (int i = 0; i < lenth; i++)
        handleWrite({ProtoBuf::Method::Post, path, data.at(i)});
}

void app::TcpClient::handleDelete(const std::filesystem::path &path) {
    handleWrite({ProtoBuf::Method::Delete, path,
                 std::vector<char>{'n', 'u', 'l', 'l'}});
};

void app::TcpClient::connect() {
    tcpSocket.async_connect(ep, [this](const asio::system_error &e) {
        if (e.code()) {
            debug("connect failed");
            connect();
            return;
        }
        connectFlag = true;
        debug("connect success");

        handleRead();
    });
}

void app::TcpClient::disconnect() {
    if (connectFlag) {
        connectFlag = false;
        tcpSocket.shutdown(asio::ip::tcp::socket::shutdown_both);
        tcpSocket.close();
        io.stop();
        debug("disconnect success");
    } else {
        debug("client is disconnect,disconnect fail");
    }
}

bool app::TcpClient::isConnected() { return connectFlag; }

std::string app::TcpClient::getResult() { return result; }

void app::TcpClient::run() {
    std::thread([this]() {
        asio::io_context::work work(io);
        try {
            io.run();
        } catch (const asio::system_error &e) {
            error(e.what());
        }
    }).detach();
}
