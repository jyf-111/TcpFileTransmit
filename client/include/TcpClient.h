#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <filesystem>
#include <memory>
#include <vector>

#include "WriteSession.h"
#include "asio/io_context.hpp"

using namespace spdlog;

namespace app {
/**
 * @brief TcpClient
 */
class TcpClient : public std::enable_shared_from_this<TcpClient> {
    std::string domain;
    std::string ip = "127.0.0.1";
    std::size_t port = 8000;
    std::size_t filesplit = 65536 * 3;

    std::string result;
    std::vector<std::string> dirList;
    std::filesystem::path selectPath = ".";
    std::string savePath = ".";
    bool connectFlag = false;

    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<asio::io_service> io;
    std::shared_ptr<asio::io_context::strand> fileWriteStrand;
    std::shared_ptr<asio::ssl::context> ssl_context;
    std::shared_ptr<asio::steady_timer> timer;
    std::shared_ptr<ssl_socket> socketPtr;
    std::shared_ptr<WriteSession> session;
    std::shared_ptr<asio::ip::tcp::resolver> resolver;
    /**
     * convert dir string to list
     */
    void ConvertDirStringToList(const std::string &);

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
    TcpClient(std::shared_ptr<asio::io_context> io);
    std::shared_ptr<asio::io_context> getIoContext();
    [[nodiscard]] std::string getIp() const;
    void setIp(const std::string &);
    [[nodiscard]] std::string getDomain() const;
    void setDomain(const std::string &);
    [[nodiscard]] std::size_t getPort() const;
    void setPort(const std::size_t &);
    [[nodiscard]] std::size_t getFilesplitsize() const;
    void setFilesplit(const std::size_t &);
    void setResult(const std::string &);
    std::string getResult();
    void setDirList(const std::vector<std::string> &dir);
    const std::vector<std::string> &getDirList();
    void setSavePath(const std::string &savePath);
    const std::string getSavePath();

    void handleQuery(const std::filesystem::path &);
    void handleGet(const std::filesystem::path &);
    void handlePost(const std::filesystem::path &,
                    const std::vector<std::vector<char>> &);
    void handleDelete(const std::filesystem::path &);

    void connect();
    void disconnect();
    bool isConnected();
};

}  // namespace app
