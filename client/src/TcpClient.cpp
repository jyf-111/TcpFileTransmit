#include "TcpClient.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <filesystem>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"

app::TcpClient::TcpClient(std::shared_ptr<asio::io_context> io) : io(io) {
    logger = spdlog::get("logger");
    assert(logger != nullptr);

    const auto &value = Properties::readProperties();
    domain = value["domain"].asString();
    ip = value["ip"].asString();
    port = value["port"].asLargestUInt();
    filesplit = value["filesplit"].asLargestUInt();

    logger->info("domain: {}, ip: {}, port: {}, filesplit: {}", domain, ip,
                 port, filesplit);

    timer = std::make_unique<asio::steady_timer>(*io, std::chrono::seconds(3));
    resolver = std::make_unique<asio::ip::tcp::resolver>(*io);
}

std::shared_ptr<asio::io_context> app::TcpClient::getIoContext() { return io; }

const std::string &app::TcpClient::getIp() const { return ip; }

void app::TcpClient::setIp(const std::string &ip) { this->ip = std::move(ip); }

const std::string &app::TcpClient::getDomain() const { return this->domain; }

void app::TcpClient::setDomain(const std::string &domain) {
    this->domain = std::move(domain);
}

const std::size_t &app::TcpClient::getPort() const { return port; }

void app::TcpClient::setPort(const std::size_t &port) { this->port = port; }

const std::size_t &app::TcpClient::getFilesplitsize() const {
    return filesplit;
}

void app::TcpClient::setFilesplit(const std::size_t &size) {
    this->filesplit = size;
}

const std::vector<std::pair<std::string, std::size_t>>
    &app::TcpClient::getDirList() {
    return dirList;
}

void app::TcpClient::setDirList(
    const std::vector<std::pair<std::string, std::size_t>> &dir) {
    this->dirList = std::move(dir);
}

void app::TcpClient::setqueryPath(const std::filesystem::path &selectPath) {
    this->queryPath = std::move(selectPath);
}

[[nodiscard]] const std::filesystem::path &app::TcpClient::getqueryPath() {
    return this->queryPath;
}

void app::TcpClient::clearDirList() { dirList.clear(); }

void app::TcpClient::setSavePath(const std::filesystem::path &savePath) {
    this->savePath = std::move(savePath);
}

const std::filesystem::path &app::TcpClient::getSavePath() { return savePath; }

void app::TcpClient::ConvertDirStringToList(const std::string &dir) {
    std::stringstream ss(dir);

    std::string filename;
    std::size_t filesize;
    while (ss >> filename >> filesize) {
        this->dirList.emplace_back(std::move(filename), filesize);
    };
}

void app::TcpClient::handleQuery(const std::filesystem::path &path) {
    queryPath = std::move(path);
}

void app::TcpClient::handleGet(const std::filesystem::path &path,
                               const std::filesystem::path &savepath) {
    ProtoBuf protoBuf{ProtoBuf::Method::Get, path,
                      std::vector<char>{'n', 'u', 'l', 'l'}};
    const auto &tmpFile =
        savepath.string() + "/" + path.filename().string() + ".sw";
    if (File::FileIsExist(tmpFile)) {
        logger->info("swap file is exist: {}", tmpFile);
        protoBuf.SetIndex(File::GetFileSize(tmpFile));
    }
    session->enqueue(protoBuf);
}

void app::TcpClient::handlePost(const std::filesystem::path &selectPath,
                                const std::filesystem::path &sendtoPath) {
    io->post([selectPath, sendtoPath, self = shared_from_this()]() {
        const auto &size =
            File::GetRemoteFileSize(selectPath.string() + ".sw", self->dirList);
        const auto &data =
            File::GetFileDataSplited(selectPath, size, self->filesplit);

        const auto &lenth = data.size();
        for (int i = 0; i < lenth; ++i) {
            ProtoBuf protobuf{ProtoBuf::Method::Post, sendtoPath, data.at(i)};
            protobuf.SetIndex(i);
            protobuf.SetTotal(lenth - 1);
            self->session->enqueue(protobuf);
        }
    });
}

void app::TcpClient::handleDelete(const std::filesystem::path &path) {
    session->enqueue({ProtoBuf::Method::Delete, path,
                      std::vector<char>{'n', 'u', 'l', 'l'}});
};

void app::TcpClient::connect() {
    if (domain.empty()) {
        ipConnect();
    } else {
        domainConnect();
    }
}

void app::TcpClient::domainConnect() {
    assert(!domain.empty());
    resolver->async_resolve(
        domain, std::to_string(port),
        [self = shared_from_this()](const asio::error_code &e,
                                    asio::ip::tcp::resolver::iterator iter) {
            if (e) {
                self->logger->warn("{}", e.message());
                self->timer->async_wait([self](const asio::error_code &e) {
                    self->timer->expires_from_now(std::chrono::seconds(3));
                    self->domainConnect();
                });
                return;
            }
            asio::ip::tcp::resolver::iterator end;
            if (iter != end) {
                self->setIp(iter->endpoint().address().to_string());
                self->logger->info("resolve ip: {}", self->ip);
                self->ipConnect();
            }
        });
}

void app::TcpClient::ipConnect() {
    socketPtr = std::make_shared<ssl_socket>(*io, ssl_context);
    session = std::make_shared<ClientSession>(socketPtr, io);
    session->initClient(shared_from_this());

    logger->info("connectting");

    socketPtr->next_layer().async_connect(
        asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port),
        [self = shared_from_this()](const asio::system_error &e) {
            if (e.code()) {
                self->logger->warn("connect {}:{} failed: {}", self->ip,
                                   self->port, e.what());
                self->timer->async_wait([self](const asio::system_error &e) {
                    // NOTE:
                    // windows 20s
                    // linux 127s
                    self->ipConnect();
                });
                return;
            }
            self->logger->info("connect {}:{} success ", self->ip, self->port);

            self->socketPtr->async_handshake(
                asio::ssl::stream_base::client,
                [self](const asio::system_error &e) {
                    if (e.code()) {
                        self->logger->error("handshake failed: {}", e.what());
                        return;
                    }
                    self->logger->info("handshake success");

                    const std::size_t &size =
                        std::numeric_limits<std::size_t>().max();
                    asio::socket_base::send_buffer_size send_size(size);
                    asio::socket_base::receive_buffer_size receive_size(size);
                    self->socketPtr->next_layer().set_option(send_size);
                    self->socketPtr->next_layer().set_option(receive_size);

                    self->connectFlag = true;
                    self->session->registerQuery();
                    self->session->doWrite();
                    self->session->doRead();
                });
        });
}

void app::TcpClient::disconnect() {
    if (connectFlag) {
        connectFlag = false;
        socketPtr->async_shutdown(
            [self = shared_from_this()](const asio::system_error &e) {
                if (e.code()) {
                    self->logger->error("shutdown failed: {}", e.what());
                    return;
                }
                self->logger->info("shutdown success");
                self->socketPtr->next_layer().close();
            });
    } else {
        logger->info("client is disconnect");
    }
}

bool app::TcpClient::isConnected() { return connectFlag; }
