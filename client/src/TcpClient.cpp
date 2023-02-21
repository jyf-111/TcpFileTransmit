#include "TcpClient.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "File.h"
#include "Properties.h"
#include "ProtoBuf.h"

using namespace spdlog;

std::string app::TcpClient::getIp() const { return ip; }

void app::TcpClient::setIp(const std::string &ip) { this->ip = ip; }

std::string app::TcpClient::getDomain() const { return this->domain; }

void app::TcpClient::setDomain(const std::string &domain) {
    this->domain = domain;

    if (domain.size() != 0) {
        resolver.async_resolve(
            domain, std::to_string(port),
            [self = shared_from_this()](
                const asio::error_code &e,
                asio::ip::tcp::resolver::iterator iter) {
                if (e) {
                    error("query Error: {}", e.message());
                    return;
                }
                self->setIp(iter->endpoint().address().to_string());
            });
    }
}

[[nodiscard]] std::size_t app::TcpClient::getPort() const { return port; }

void app::TcpClient::setPort(const std::size_t &port) { this->port = port; }

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

void app::TcpClient::handleOutPutTime(std::string &result) {
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

void app::TcpClient::handleRead() {
    if (!connectFlag) throw std::runtime_error("not connected");
    auto resultBuf = std::make_shared<asio::streambuf>();
    auto streambuf = std::make_shared<asio::streambuf>();
    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();

    asio::async_read(
        tcpSocket, *streambuf,
        [peek, streambuf](const asio::system_error &e,
                          std::size_t size) -> std::size_t {
            if (e.code()) {
                error("async_reading: {}", e.what());
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
                error("async_read: {}", e.message());
                self->connect();
                return;
            }
            debug("read complete");

            self->handleRead();

            ProtoBuf protoBuf;
            std::istream is(streambuf.get());
            is >> protoBuf;

            if (ProtoBuf::Method::Post == protoBuf.GetMethod()) {
                if (protoBuf.GetIsFile()) {
                    File file(self->savePath + "/" +
                              protoBuf.GetPath().filename().string());
                    file.SetFileData(protoBuf.GetData());
                    const auto &index = protoBuf.GetIndex();
                    const auto &total = protoBuf.GetTotal();
                    self->result.clear();
                    if (index < total) {
                        self->result +=
                            "get file: " + protoBuf.GetPath().string() + " " +
                            std::to_string(index) + "/" + std::to_string(total);
                    } else if (index == total) {
                        self->result +=
                            "get file: " + protoBuf.GetPath().string() + " " +
                            std::to_string(index) + "/" +
                            std::to_string(total) + " ok";
                    }
                    self->handleOutPutTime(self->result);
                } else {
                    const auto &data = protoBuf.GetData();
                    if (protoBuf.GetIsDir()) {
                        self->dir.clear();
                        self->dir += std::string(data.begin(), data.end());
                    } else {
                        self->result.clear();
                        self->handleOutPutTime(self->result);
                        self->result += std::string(data.begin(), data.end());
                    }
                }
            } else {
                error("recv protobuf`s method is not post");
            }
        });
}

void app::TcpClient::handleWrite(const ProtoBuf &protobuf) {
    if (!connectFlag) {
        result = "not connected";
        error("{}", result);
        return;
    }

    debug("new write");
    auto buf = std::make_shared<asio::streambuf>();
    auto os = std::make_shared<std::ostream>(buf.get());
    *os << protobuf;

    // NOTE: write
    asio::write(tcpSocket, *buf.get());
}

void app::TcpClient::registerQuery() {
    if (connectFlag == false) return;
    timer.async_wait([self = shared_from_this()](const asio::error_code &e) {
        if (e) {
            error("{}", e.message());
            return;
        }
        self->handleWrite({ProtoBuf::Method::Query, self->selectPath,
                           std::vector<char>{'n', 'u', 'l', 'l'}});
        self->timer.expires_from_now(std::chrono::seconds(1));
        self->registerQuery();
    });
}

void app::TcpClient::setSavePath(const std::string &savePath) {
    this->savePath = savePath;
}

void app::TcpClient::handleQuery(const std::filesystem::path &path) {
    selectPath = path;
}

void app::TcpClient::handleGet(const std::filesystem::path &path) {
    handleWrite(
        {ProtoBuf::Method::Get, path, std::vector<char>{'n', 'u', 'l', 'l'}});
}

void app::TcpClient::handlePost(const std::filesystem::path &path,
                                const std::vector<std::vector<char>> &data) {
    const auto lenth = data.size();
    for (int i = 0; i < lenth; ++i) {
        ProtoBuf protobuf{ProtoBuf::Method::Post, path, data.at(i)};
        protobuf.SetIndex(i);
        protobuf.SetTotal(lenth - 1);
        handleWrite(protobuf);
    }
}

void app::TcpClient::handleDelete(const std::filesystem::path &path) {
    handleWrite({ProtoBuf::Method::Delete, path,
                 std::vector<char>{'n', 'u', 'l', 'l'}});
};

void app::TcpClient::connect() {
    debug("connectting");
    static asio::ip::tcp::endpoint ep =
        asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port);
    ;
    tcpSocket.async_connect(ep, [this](const asio::system_error &e) {
        if (e.code()) {
            warn("connect {}:{} failed: {}", ip, port, e.what());
            // NOTE:
            // windows 20s
            // linux 127s
            connect();
            return;
        }
        connectFlag = true;
        info("connect {}:{} success ", ip, port);
        handleRead();
        registerQuery();
    });
}

void app::TcpClient::disconnect() {
    if (connectFlag) {
        connectFlag = false;
        tcpSocket.close();
        info("disconnect success");
    } else {
        info("client is disconnect");
    }
}

bool app::TcpClient::isConnected() { return connectFlag; }

std::string app::TcpClient::getResult() { return result; }

std::string app::TcpClient::getDir() { return dir; }

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
