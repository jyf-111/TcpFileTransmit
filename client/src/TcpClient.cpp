#include "TcpClient.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"

app::TcpClient::TcpClient(std::shared_ptr<asio::io_context> io) : io(io) {
    logger = spdlog::get("logger");
    fileWriteStrand = std::make_unique<asio::io_context::strand>(*io);
    timer = std::make_unique<asio::steady_timer>(*io, std::chrono::seconds(3));
    resolver = std::make_unique<asio::ip::tcp::resolver>(*io);
}

std::shared_ptr<asio::io_context> app::TcpClient::getIoContext() { return io; }

std::string app::TcpClient::getIp() const { return ip; }

void app::TcpClient::setIp(const std::string &ip) { this->ip = ip; }

std::string app::TcpClient::getDomain() const { return this->domain; }

void app::TcpClient::setDomain(const std::string &domain) {
    this->domain = domain;

    if (domain.size() != 0) {
        try {
            auto iter = resolver->resolve(domain, std::to_string(port));
            setIp(iter->endpoint().address().to_string());
        } catch (const asio::error_code &e) {
            logger->error("{}", e.message());
        }
    }
}

std::size_t app::TcpClient::getPort() const { return port; }

void app::TcpClient::setPort(const std::size_t &port) { this->port = port; }

void app::TcpClient::setFilesplit(const std::size_t &size) {
    this->filesplit = size;
}

std::size_t app::TcpClient::getFilesplitsize() const { return filesplit; }

void app::TcpClient::setResult(const std::string &result) {
    this->result = result;
}

std::string app::TcpClient::getResult() { return handleOutPutTime(result); }

void app::TcpClient::setDirList(
    const std::vector<std::pair<std::string, std::size_t>> &dir) {
    this->dirList = std::move(dir);
}

const std::vector<std::pair<std::string, std::size_t>>
    &app::TcpClient::getDirList() {
    return dirList;
}

void app::TcpClient::setSavePath(const std::string &savePath) {
    this->savePath = savePath;
}

const std::string app::TcpClient::getSavePath() { return savePath; }

void app::TcpClient::ConvertDirStringToList(const std::string &dir) {
    std::stringstream ss(dir);
    std::string item;
    int i = 0;
    while (std::getline(ss, item, '\n')) {
        std::stringstream ss{item};
        std::string filename;
        std::size_t filesize;
        ss >> filename >> filesize;
        this->dirList.emplace_back(filename, filesize);
    };
}

const std::string app::TcpClient::handleOutPutTime(const std::string &result) {
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    std::string ret;
    ret += "[";
    ret += std::to_string(now->tm_hour);
    ret += ":";
    ret += std::to_string(now->tm_min);
    ret += ":";
    ret += std::to_string(now->tm_sec);
    ret += "]\n";
    ret.append(result);
    ret.append("\n");
    return ret;
}

void app::TcpClient::handleRead() {
    if (!connectFlag) throw std::runtime_error("not connected");
    auto resultBuf = std::make_shared<asio::streambuf>();
    auto streambuf = std::make_shared<asio::streambuf>();
    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();

    asio::async_read(
        *socketPtr, *streambuf,
        [peek, streambuf, self = shared_from_this()](
            const asio::system_error &e, std::size_t size) -> std::size_t {
            if (e.code()) {
                self->logger->error("async_reading: {}", e.what());
                return 0;
            }
            if (size == sizeof(std::size_t)) {
                std::memcpy(peek.get(), streambuf.get()->data().data(),
                            sizeof(std::size_t));
            }
            if (size > sizeof(std::size_t) &&
                size == *reinterpret_cast<std::size_t *>(peek.get())) {
                return 0;
            } else {
                return 1;
            }
        },
        [streambuf, self = shared_from_this()](const asio::error_code &e,
                                               std::size_t size) {
            if (e) {
                self->disconnect();
                self->logger->error("async_read: {}", e.message());
                self->connect();
                return;
            }
            self->logger->debug("read complete");

            self->handleRead();

            ProtoBuf protoBuf;
            std::istream is(streambuf.get());
            is >> protoBuf;

            if (ProtoBuf::Method::Post == protoBuf.GetMethod()) {
                if (protoBuf.GetIsFile()) {
                    self->fileWriteStrand->post([self, protoBuf]() {
                        File file(self->savePath + "/" +
                                  protoBuf.GetPath().filename().string() +
                                  ".sw");
                        file.SetFileData(protoBuf.GetData());
                    });
                    const auto &index = protoBuf.GetIndex();
                    const auto &total = protoBuf.GetTotal();
                    self->result.clear();
                    if (index < total) {
                        self->result =
                            "get file: " + protoBuf.GetPath().string() + " " +
                            std::to_string(index) + "/" + std::to_string(total);
                    } else if (index == total) {
                        self->fileWriteStrand->post([self, protoBuf]() {
                            const auto &tmp =
                                self->savePath + "/" +
                                protoBuf.GetPath().filename().string();
                            File::ReNameFile(tmp + ".sw", tmp);
                        });

                        self->result =
                            "get file: " + protoBuf.GetPath().string() + " " +
                            std::to_string(index) + "/" +
                            std::to_string(total) + " ok";
                    }
                } else {
                    const auto &data = protoBuf.GetData();
                    if (protoBuf.GetIsDir()) {
                        self->dirList.clear();
                        self->ConvertDirStringToList(
                            std::string(data.begin(), data.end()));
                    } else {
                        self->result.clear();
                        self->result = std::string(data.begin(), data.end());
                    }
                }
            } else {
                self->logger->error("recv protobuf`s method is not post");
            }
        });
}

void app::TcpClient::registerQuery() {
    if (connectFlag == false) return;
    timer->async_wait([self = shared_from_this()](const asio::error_code &e) {
        if (e) {
            self->logger->error("{}", e.message());
            return;
        }
        self->session->enqueue({ProtoBuf::Method::Query, self->selectPath,
                                std::vector<char>{'n', 'u', 'l', 'l'}});
        self->timer->expires_from_now(std::chrono::seconds(3));
        self->registerQuery();
    });
}

void app::TcpClient::handleQuery(const std::filesystem::path &path) {
    selectPath = path;
}

void app::TcpClient::handleGet(const std::filesystem::path &path,
                               const std::filesystem::path &savepath) {
    ProtoBuf protoBuf{ProtoBuf::Method::Get, path,
                      std::vector<char>{'n', 'u', 'l', 'l'}};
    const auto tmpFile =
        savepath.string() + "/" + path.filename().string() + ".sw";
    if (File::FileIsExist(tmpFile)) {
        logger->info("swap file is exist: {}", tmpFile);
        protoBuf.SetIndex(File::GetFileSize(tmpFile));
    }
    session->enqueue(protoBuf);
}

void app::TcpClient::handlePost(const std::filesystem::path &path,
                                const std::vector<std::vector<char>> &data) {
    const auto lenth = data.size();
    for (int i = 0; i < lenth; ++i) {
        ProtoBuf protobuf{ProtoBuf::Method::Post, path, data.at(i)};
        protobuf.SetIndex(i);
        protobuf.SetTotal(lenth - 1);
        session->enqueue(protobuf);
    }
}

void app::TcpClient::handleDelete(const std::filesystem::path &path) {
    session->enqueue({ProtoBuf::Method::Delete, path,
                      std::vector<char>{'n', 'u', 'l', 'l'}});
};

void app::TcpClient::connect() {
    socketPtr = std::make_shared<ssl_socket>(*io, ssl_context);
    session = std::make_shared<WriteSession>(socketPtr, io);

    logger->info("connectting");
    result = fmt::format("client is connectting");

    socketPtr->next_layer().async_connect(
        asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port),
        [self = shared_from_this()](const asio::system_error &e) {
            if (e.code()) {
                self->logger->warn("connect {}:{} failed: {}", self->ip,
                                   self->port, e.what());
                self->result = fmt::format("connect {}:{} failed: {}", self->ip,
                                           self->port, e.what());
                self->timer->async_wait([self](const asio::system_error &e) {
                    // NOTE:
                    // windows 20s
                    // linux 127s
                    self->connect();
                });
                return;
            }
            self->logger->info("connect {}:{} success ", self->ip, self->port);
            self->result =
                fmt::format("connect {}:{} success ", self->ip, self->port);
            self->socketPtr->async_handshake(
                asio::ssl::stream_base::client,
                [self](const asio::system_error &e) {
                    if (e.code()) {
                        self->logger->error("handshake failed: {}", e.what());
                        self->result =
                            fmt::format("handshake failed: {}", e.what());
                        return;
                    }
                    self->logger->info("handshake success");
                    self->result = fmt::format("handshake success");
                    self->connectFlag = true;
                    self->session->doWrite();
                    self->handleRead();
                    self->registerQuery();
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
                    self->result = fmt::format("shutdown failed: {}", e.what());
                    return;
                }
                self->logger->info("shutdown success");
                self->result = fmt::format("shutdown success");
                self->socketPtr->next_layer().close();
            });
    } else {
        logger->info("client is disconnect");
        result = fmt::format("client is disconnect");
    }
}

bool app::TcpClient::isConnected() { return connectFlag; }
