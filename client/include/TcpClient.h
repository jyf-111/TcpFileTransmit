#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <filesystem>
#include <memory>
#include <vector>

#include "ClientSession.h"

using namespace spdlog;

namespace app {

class TcpClient : public std::enable_shared_from_this<TcpClient> {
    std::string domain;
    std::string ip;
    std::size_t port;
    std::size_t filesplit;

    std::vector<std::pair<std::string, std::size_t>> dirList;
    std::filesystem::path queryPath = ".";
    std::string savePath = ".";
    bool connectFlag = false;

    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<asio::io_service> io;
    std::unique_ptr<asio::steady_timer> timer;
    std::unique_ptr<asio::ip::tcp::resolver> resolver;
    std::shared_ptr<ssl_socket> socketPtr;
    std::shared_ptr<ClientSession> session;
    asio::ssl::context ssl_context{asio::ssl::context::tls};

   public:
    TcpClient(std::shared_ptr<asio::io_context> io);
    std::shared_ptr<asio::io_context> getIoContext();
    [[nodiscard]] const std::string& getIp() const;
    void setIp(const std::string &);
    [[nodiscard]] const std::string& getDomain() const;
    void setDomain(const std::string &);
    [[nodiscard]] const std::size_t& getPort() const;
    void setPort(const std::size_t &);
    [[nodiscard]] const std::size_t& getFilesplitsize() const;
    void setFilesplit(const std::size_t &);
    [[nodiscard]] const std::vector<std::pair<std::string, std::size_t>>
        &getDirList();
    void setDirList(
        const std::vector<std::pair<std::string, std::size_t>> &dir);
    [[nodiscard]] const std::string& getSavePath();
    void setSavePath(const std::string &savePath);
    [[nodiscard]] const std::filesystem::path &getqueryPath();
    void setqueryPath(const std::filesystem::path &);

    void handleQuery(const std::filesystem::path &);
    void handleGet(const std::filesystem::path &,
                   const std::filesystem::path &);
    void handlePost(const std::filesystem::path &,
                    const std::filesystem::path &);
    void handleDelete(const std::filesystem::path &);

    void ConvertDirStringToList(const std::string &);
    void clearDirList();
    void connect();
    void domainConnect();
    void ipConnect();
    void disconnect();
    bool isConnected();
};

}  // namespace app
