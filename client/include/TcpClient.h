// FIXME: 不能分离编译
#pragma once
#include <spdlog/spdlog.h>
#include <vcruntime.h>
#include <vcruntime_typeinfo.h>

#include <array>
#include <asio.hpp>
#include <exception>
#include <filesystem>
#include <iostream>
#include <system_error>

#include "ProtoBuf.h"

using namespace spdlog;

namespace app {
/**
 * @brief TcpClient
 */
class TcpClient {
    asio::io_service io;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::socket tcpSocket;
    /**
     * main Process for recv and send
     */
    [[nodiscard]] std::string Process(const ProtoBuf &protobuf);

   public:
    std::string handleGet(const std::filesystem::path &path);
    std::string handlePost(const std::filesystem::path &path,
                           const std::string data);
    std::string handleDelete(const std::filesystem::path &path);

    TcpClient(std::string ip, size_t port);
};
}  // namespace app

app::TcpClient::TcpClient(std::string ip, size_t port)
    : ep(asio::ip::address::from_string(ip), port), tcpSocket(io) {
    set_level(spdlog::level::debug);
    // TODO: connect
    tcpSocket.connect(ep);
    debug("connect success");
}

inline std::string app::TcpClient::Process(const ProtoBuf &protobuf) {
    asio::streambuf buf;
    std::ostream os(&buf);
    os << protobuf;
    std::cout << protobuf << std::endl;

    asio::write(tcpSocket, buf);
    debug("Send success");

    asio::streambuf result;
    try {
        asio::read(tcpSocket, result);
    } catch (std::exception &e) {
            debug("Receive success {} byte", result.size());
            std::istream is(&result);
            std::string str, tmp;
            while (is >> tmp) {
                str += tmp +"\n";
            }
            return str;
    }
    return "";
}

std::string app::TcpClient::handleGet(const std::filesystem::path &path) {
    if (path.empty()) {
        throw std::runtime_error("path is empty");
    }

    ProtoBuf pb(ProtoBuf::Method::Get, path, "null");
    debug("path: {}", pb.GetPath().string());

    return Process(pb);
}

std::string app::TcpClient::handlePost(const std::filesystem::path &path,
                                       const std::string data) {
    if (path.empty()) {
        throw std::runtime_error("path is empty");
    }

    ProtoBuf pb;
    pb.SetMethod(ProtoBuf::Method::Post);
    pb.SetPath(path);
    pb.SetData(data);

    debug("path: {}", pb.GetPath().string());
    return Process(pb);
}

std::string app::TcpClient::handleDelete(const std::filesystem::path &path) {
    if (path.empty()) {
        throw std::runtime_error("path is empty");
    }

    ProtoBuf pb;
    pb.SetMethod(ProtoBuf::Method::Delete);
    pb.SetPath(path);

    debug("path: {}", pb.GetPath().string());
    return Process(pb);
};
