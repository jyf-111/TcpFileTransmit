#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <filesystem>
#include <memory>
#include <vector>

#include "WriteSession.h"

using namespace spdlog;

namespace app {
/**
 * @brief TcpClient
 */
class TcpClient : public std::enable_shared_from_this<TcpClient> {
    asio::io_service io;
    asio::steady_timer timer{io, std::chrono::seconds(3)};
    std::string domain;
    std::shared_ptr<asio::ip::tcp::socket> socketPtr =
        std::make_shared<asio::ip::tcp::socket>(io);
    std::shared_ptr<WriteSession> session =
        std::make_shared<WriteSession>(socketPtr);

    asio::ip::tcp::resolver resolver{io};
    std::string result;
    std::string dir;
    std::filesystem::path selectPath = ".";
    std::string savePath = ".";
    bool connectFlag = false;

    std::string ip = "127.0.0.1";
    std::size_t port = 8000;
    std::string level = "info";
    std::size_t filesplit = 65536 * 3;

    /**
     * handle read
     */
    void handleRead();

    /**
     * @brief add time to result
     */
    void handleOutPutTime(std::string &);
    /**
     * register query
     */
    void registerQuery();

   public:
    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &);
    [[nodiscard]] std::string getDomain() const;
    void setDomain(const std::string &);
    [[nodiscard]] std::size_t getPort() const;
    void setPort(const std::size_t &);
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
    std::string getDir();
    void setSavePath(const std::string &savePath);

    void run();
};

}  // namespace app
