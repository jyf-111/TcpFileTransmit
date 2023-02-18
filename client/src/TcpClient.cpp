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

std::string app::TcpClient::getIp() const { return ip; }

void app::TcpClient::setIp(const std::string &ip) { this->ip = ip; }

[[nodiscard]] std::size_t app::TcpClient::getPort() const { return port; }

void app::TcpClient::setPort(const size_t &port) { this->port = port; }

[[nodiscard]] std::string app::TcpClient::getLevel() const { return level; }

void app::TcpClient::setFilesplit(const std::size_t &size) {
    this->filesplit = size;
}

[[nodiscard]] std::size_t app::TcpClient::getFilesplitsize() const {
    return filesplit;
}

void app::TcpClient::setLevel(const std::string &level) {
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

void app::TcpClient::readProperties() {
    try {
        Properties properties;
        auto value = properties.readProperties();
        setIp(value["ip"].asString());
        setPort(value["port"].asUInt());
        setLevel(value["log"].asString());
        setFilesplit(value["splitsize"].asUInt());
    } catch (std::exception &e) {
        warn("{}", e.what());
    }

    ep = asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
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
                connect();
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
    writeStrand.post([this, buf]() {
        asio::async_write(tcpSocket, *buf.get(),
                          [this](const asio::error_code &e, size_t size) {
                              if (e) {
                                  error("{}", e.message());
                                  return;
                              }
                              debug("write complete");
                          });
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
    debug("connectting");
    tcpSocket.async_connect(ep, [this](const asio::system_error &e) {
        if (e.code()) {
            warn("connect failed");
            connect();
            return;
        }
        connectFlag = true;
        info("connect success");

        handleRead();
    });
}

void app::TcpClient::disconnect() {
    if (connectFlag) {
        connectFlag = false;
        tcpSocket.shutdown(asio::ip::tcp::socket::shutdown_both);
        tcpSocket.close();
        info("disconnect success");
    } else {
        info("client is disconnect,disconnect fail");
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
