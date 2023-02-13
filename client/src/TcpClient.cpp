#include "TcpClient.h"

#include "Properties.h"
#include "asio/system_error.hpp"

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

void app::TcpClient::resultHandle(std::string &result) {
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    std::string tmp;
    tmp += "[";
    tmp += std::to_string(now->tm_hour);
    tmp += ":";
    tmp += std::to_string(now->tm_min);
    tmp += ":";
    tmp += std::to_string(now->tm_sec);
    tmp += "]\n";
    tmp.append(result);
    tmp.append("\n");
    result = tmp;
}

void app::TcpClient::handleReadAndWrite(const ProtoBuf protobuf) {
    if (!connectFlag) {
        result = "not connected";
        error("{}", result);
        return;
    }

    io.post([this, protobuf]() {
        debug("new read and write");
        std::shared_ptr<asio::streambuf> buf =
            std::make_shared<asio::streambuf>();
        std::shared_ptr<std::ostream> os =
            std::make_shared<std::ostream>(buf.get());
        *os << protobuf;
        debug("Send: {} {} {}", ProtoBuf::MethodToString(protobuf.GetMethod()),
              protobuf.GetPath().string(), protobuf.GetData());

        // NOTE: async_write
        asio::async_write(
            tcpSocket, *buf, [this](const asio::error_code e, size_t size) {
                if (e) throw e;
                debug("Send success");

                std::shared_ptr<asio::streambuf> resultBuf =
                    std::make_shared<asio::streambuf>();
                asio::async_read_until(
                    tcpSocket, *resultBuf, '\n',
                    [resultBuf, this](const asio::error_code &e, size_t size) {
                        debug("Read success");
                        std::istream is(resultBuf.get());
                        std::getline(is, result);
                    });
            });
    });
}

void app::TcpClient::handleGet(const std::filesystem::path &path) {
    if (path.empty()) {
        throw std::runtime_error("path is empty");
    }
    handleReadAndWrite({ProtoBuf::Method::Get, path, "null"});
}

void app::TcpClient::handlePost(const std::filesystem::path &path,
                                const std::string data) {
    if (path.empty()) {
        throw std::runtime_error("path is empty");
    }
    handleReadAndWrite({ProtoBuf::Method::Post, path, data});
}

void app::TcpClient::handleDelete(const std::filesystem::path &path) {
    if (path.empty()) {
        throw std::runtime_error("path is empty");
    }
    handleReadAndWrite({ProtoBuf::Method::Delete, path, "null"});
};

void app::TcpClient::connect() {
    tcpSocket.async_connect(ep, [this](const asio::system_error &e) {
        connectFlag = true;
        debug("connect success");
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

std::string app::TcpClient::getResult() {
    std::replace(result.begin(), result.end(), ' ', '\n');
    return result;
}

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
