#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <filesystem>
#include <vector>

#include "ProtoBuf.h"
#include "asio/io_context.hpp"

using namespace spdlog;

namespace app {
/**
 * @brief TcpClient
 */
class TcpClient {
    asio::io_service io;
    asio::ip::tcp::endpoint ep;
    asio::ip::tcp::socket tcpSocket{io};
    asio::io_context::strand writeStrand{io};
    std::string result;
    bool connectFlag = false;

    std::string ip = "127.0.0.1";
    size_t port = 8000;
    std::string level = "info";
    std::size_t filesplit = 65536 * 3;

    /**
     * handle read
     */
    void handleRead();

    /**
     * main Process for recv and send
     */
    void handleWrite(const ProtoBuf &protobuf);

    /**
     * @brief add time to result
     */
    void handleResult(std::string &);

   public:
    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &);
    [[nodiscard]] std::size_t getPort() const;
    void setPort(const size_t &);
    [[nodiscard]] std::string getLevel() const;
    void setLevel(const std::string &);
    [[nodiscard]] std::size_t getFilesplitsize() const;
    void setFilesplit(const std::size_t &);

    void handleQuery(const std::filesystem::path &);
    void handleGet(const std::filesystem::path &);
    void handlePost(const std::filesystem::path &,
                    const std::vector<std::vector<char>> &);
    void handleDelete(const std::filesystem::path &);

    void connect();
    void disconnect();
    bool isConnected();
    std::string getResult();

    void run();
};

}  // namespace app
