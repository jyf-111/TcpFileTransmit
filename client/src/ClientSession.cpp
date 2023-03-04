#include "ClientSession.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <queue>

#include "File.h"
#include "Properties.h"
#include "TcpClient.h"
#include "asio/error_code.hpp"

ClientSession::ClientSession(std::shared_ptr<ssl_socket> socketPtr,
                             std::shared_ptr<asio::io_context> io)
    : socketPtr(std::move(socketPtr)) {
    timer = std::make_shared<asio::steady_timer>(*io);
    queryTimer = std::make_shared<asio::steady_timer>(*io);
    fileWriteStrand = std::make_unique<asio::io_context::strand>(*io);
    logger = spdlog::get("logger");
    assert(logger);

    const auto &value = Properties::readProperties();
    gaptime = value["gaptime"].asLargestInt();

    logger->info("gaptime: {}", gaptime);
}

void ClientSession::initClient(std::weak_ptr<app::TcpClient> client) {
    this->client = client.lock();
    assert(this->client);
}

bool ClientSession::queryIsEmpty() {
    std::lock_guard<std::mutex> lock(mtx);
    return writeQueue.empty();
}

const ProtoBuf ClientSession::popQueryFront() {
    std::lock_guard<std::mutex> lock(mtx);
    const auto query = writeQueue.front();
    writeQueue.pop();
    return query;
}

void ClientSession::registerQuery() {
    if (client->isConnected() == false) return;
    queryTimer->async_wait(
        [self = shared_from_this()](const asio::error_code &e) {
            if (e) {
                self->logger->error("{}", e.message());
                return;
            }
            if (self->writeQueue.empty()) {
                self->enqueue({ProtoBuf::Method::Query,
                               self->client->getqueryPath(),
                               std::vector<char>{'n', 'u', 'l', 'l'}});
            }
            self->queryTimer->expires_from_now(
                std::chrono::milliseconds(self->gaptime));
            self->registerQuery();
        });
}

void ClientSession::enqueue(const ProtoBuf &buf) {
    std::lock_guard<std::mutex> lock(mtx);
    writeQueue.push(buf);
}

void ClientSession::doRead() {
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
                self->client->disconnect();
                self->logger->error("async_read: {}", e.message());
                self->client->ipConnect();
                return;
            }
            self->logger->debug("read complete");

            self->doRead();

            ProtoBuf protoBuf;
            std::istream is(streambuf.get());
            is >> protoBuf;

            if (ProtoBuf::Method::Post == protoBuf.GetMethod()) {
                if (protoBuf.GetIsFile()) {
                    self->fileWriteStrand->post([self, protoBuf]() {
                        const std::string &filename =
                            self->client->getSavePath().string() + "/" +
                            protoBuf.GetPath().filename().string() + ".sw";
                        File::SetFileData(filename, protoBuf.GetData());
                    });
                    const auto &index = protoBuf.GetIndex();
                    const auto &total = protoBuf.GetTotal();
                    if (index < total) {
                        self->logger->info(
                            "get file: " + protoBuf.GetPath().string() + " " +
                            std::to_string(index) + "/" +
                            std::to_string(total));
                    } else if (index == total) {
                        self->fileWriteStrand->post([self, protoBuf]() {
                            const auto &tmp =
                                self->client->getSavePath().string() + "/" +
                                protoBuf.GetPath().filename().string();
                            File::ReNameFile(tmp + ".sw", tmp);
                        });
                        self->logger->info(
                            "get file: " + protoBuf.GetPath().string() + " " +
                            std::to_string(index) + "/" +
                            std::to_string(total) + " ok");
                    }
                } else {
                    const auto &data = protoBuf.GetData();
                    if (protoBuf.GetIsDir()) {
                        self->client->clearDirList();
                        self->client->ConvertDirStringToList(
                            std::string(data.begin(), data.end()));
                    } else {
                        self->logger->info(
                            std::string(data.begin(), data.end()));
                    }
                }
            } else {
                self->logger->error("recv protobuf`s method is not post");
            }
        });
}

void ClientSession::doWrite() {
    if (queryIsEmpty()) {
        timer->async_wait(
            [self = shared_from_this()](const asio::error_code &e) {
                if (e) self->logger->error("async_wait: {}", e.message());
                self->timer->expires_after(
                    std::chrono::milliseconds(self->gaptime));
                self->doWrite();
            });
        return;
    }

    // NOTE: buf in doWrite to makesure thread safe
    auto buf = std::make_shared<asio::streambuf>();
    std::ostream os{buf.get()};
    os << popQueryFront();
    asio::async_write(*socketPtr, *buf,
                      [self = shared_from_this()](const asio::error_code &e,
                                                  std::size_t size) {
                          if (e)
                              self->logger->error("async_write: {}",
                                                  e.message());
                          self->doWrite();
                      });
}
