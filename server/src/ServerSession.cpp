#include "ServerSession.h"

#include <chrono>
#include <mutex>

#include "File.h"
#include "Properties.h"

ServerSession::ServerSession(std::shared_ptr<ssl_socket> socketPtr,
                             std::shared_ptr<asio::io_context> io)
    : socketPtr(std::move(socketPtr)), io(io) {
    timer = std::make_shared<asio::steady_timer>(
        *io, std::chrono::milliseconds(500));
    fileWriteStrand = std::make_shared<asio::io_context::strand>(*io);
    sig = std::make_shared<asio::signal_set>(*io, SIGINT, SIGTERM);

    logger = spdlog::get("logger");
    assert(logger);

    const auto& value = Properties::readProperties();
    filesplit = value["filesplit"].asLargestUInt();

    logger->info("filesplit: {}", filesplit);
}

void ServerSession::enqueue(const ProtoBuf& buf) {
    std::lock_guard<std::mutex> lock(mtx);
    writeQueue.push(buf);
}

void ServerSession::handleCloseSocket() {
    socketPtr->async_shutdown(
        [self = shared_from_this()](const asio::error_code& e) {
            if (e) {
                self->logger->error(e.message());
            }
            self->socketPtr->lowest_layer().close();
            std::queue<ProtoBuf> tmp;
            tmp.swap(self->writeQueue);
        });
}

void ServerSession::doRead() {
    logger->debug("new read");
    auto streambuf = std::make_shared<asio::streambuf>();
    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();

    asio::async_read(
        *socketPtr, *streambuf,
        [peek, streambuf](const asio::system_error& e,
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
                size == *reinterpret_cast<std::size_t*>(peek.get())) {
                return 0;
            } else {
                return 1;
            }
        },
        [streambuf, self = shared_from_this()](const asio::error_code& e,
                                               std::size_t size) {
            if (e) {
                self->handleCloseSocket();
                error("async_read: {}", e.message());
                return;
            }

            self->logger->debug("read complete");
            self->doRead();

            std::variant<std::string, std::vector<std::vector<char>>> result;

            ProtoBuf recv;
            try {
                std::istream is(streambuf.get());
                is >> recv;
                result = self->handleFileAction(recv);
            } catch (const std::exception& e) {
                error(e.what());
                result = std::string(e.what());
            }
            if (result.index() == 0) {
                const auto& str = std::get<std::string>(result);

                ProtoBuf send{ProtoBuf::Method::Post, recv.GetPath(),
                              std::vector<char>(str.begin(), str.end())};
                if (recv.GetMethod() == ProtoBuf::Method::Query) {
                    send.SetIsDir(true);
                }
                self->enqueue(send);
            } else {
                const auto& vec =
                    std::get<std::vector<std::vector<char>>>(result);
                const auto& len = vec.size();
                for (std::size_t i = 0; i < len; ++i) {
                    ProtoBuf ret{ProtoBuf::Method::Post, recv.GetPath(),
                                 vec.at(i)};
                    ret.SetIsFile(true);
                    ret.SetIndex(i);
                    ret.SetTotal(len - 1);
                    self->enqueue(ret);
                }
            }
        });
}

auto ServerSession::handleFileAction(ProtoBuf& protoBuf)
    -> std::variant<std::string, std::vector<std::vector<char>>> {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    switch (method) {
        case ProtoBuf::Method::Query: {
            return File::QueryDirectory(path);
        }
        case ProtoBuf::Method::Get: {
            const auto& index = protoBuf.GetIndex();
            return File::GetFileDataSplited(path, index, filesplit);
        }
        case ProtoBuf::Method::Post: {
            fileWriteStrand->post([path, data] {
                File::SetFileData(path.string() + ".sw", data);
            });
            auto index = protoBuf.GetIndex();
            auto total = protoBuf.GetTotal();
            if (index < total) {
                return "server saving file : " + std::to_string(index) + "/" +
                       std::to_string(total);
            } else if (index == total) {
                fileWriteStrand->post(
                    [path] { File::ReNameFile(path.string() + ".sw", path); });
                return "server saving file : " + std::to_string(index) + "/" +
                       std::to_string(total) + " OK";
            } else {
                error("index > total");
                return "error: index > total";
            }
        }
        case ProtoBuf::Method::Delete: {
            File::DeleteActualFile(path.string());
            return "delete file OK";
        }
        default:
            throw std::runtime_error("unknown method");
    }
    return "unknown method";
}

void ServerSession::doWrite() {
    std::lock_guard<std::mutex> lock(mtx);
    if (writeQueue.empty()) {
        timer->async_wait(
            [self = shared_from_this()](const asio::error_code& e) {
                if (e) error("async_wait: {}", e.message());
                self->timer->expires_after(std::chrono::seconds(1));
                self->doWrite();
            });
        return;
    }

    // NOTE: buf in doWrite to makesure thread safe
    auto buf = std::make_shared<asio::streambuf>();
    std::ostream os{buf.get()};
    os << writeQueue.front();
    writeQueue.pop();
    asio::async_write(*socketPtr, *buf,
                      [self = shared_from_this()](const asio::error_code& e,
                                                  std::size_t size) {
                          if (e) error("async_write: {}", e.message());
                          self->doWrite();
                      });
}

void ServerSession::registerSignal() {
    sig->async_wait([self = shared_from_this()](const std::error_code& e,
                                                int signal_number) {
        switch (signal_number) {
            case SIGINT:
                self->logger->info("SIGINT received, shutting down");
                self->handleCloseSocket();
                self->io->stop();
                break;
            case SIGTERM:
                self->logger->info("SIGTerm received, shutting down");
                self->handleCloseSocket();
                self->io->stop();
                break;
            default:
                self->logger->info("default {}", e.message());
        }
    });
}
