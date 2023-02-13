#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <filesystem>

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
    std::string result;
    bool connectFlag = false;
    /**
     * main Process for recv and send
     */
    void handleReadAndWrite(const ProtoBuf protobuf);

    /**
     * @brief add time to result
     */
    void resultHandle(std::string&);

   public:
    void handleGet(const std::filesystem::path &);
    void handlePost(const std::filesystem::path &, const std::string);
    void handleDelete(const std::filesystem::path &);

    TcpClient(std::string ip, size_t port);
    void connect();
    void disconnect();
    bool isConnected();
    std::string getResult();

    void run();
};

}  // namespace app
