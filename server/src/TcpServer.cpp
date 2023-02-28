#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"
#include "WriteSession.h"

using namespace spdlog;

TcpServer::TcpServer() {
    io = std::make_shared<asio::io_context>();
    fileWriteStrand = std::make_shared<asio::io_context::strand>(*io);
    sig = std::make_shared<asio::signal_set>(*io, SIGINT, SIGTERM);
}

std::string TcpServer::getIp() const { return ip; }

void TcpServer::setIp(const std::string& ip) { this->ip = ip; }

[[nodiscard]] std::size_t TcpServer::getPort() const { return port; }

void TcpServer::setPort(const std::size_t& port) { this->port = port; }

void TcpServer::setFilesplit(const std::size_t& size) {
    this->filesplit = size;
}

[[nodiscard]] std::size_t TcpServer::getFilesplitsize() const {
    return filesplit;
}

[[nodiscard]] std::size_t TcpServer::getThreads() const { return threads; }

void TcpServer::setThreads(const std::size_t& threads) {
    if (threads > 0) {
        this->threads = threads;
    } else {
        this->threads = std::thread::hardware_concurrency();
    }
}

[[nodiscard]] std::string TcpServer::getCertificate() const {
    return this->certificate;
}

void TcpServer::setCertificate(const std::string& certificate) {
    this->certificate = certificate;
}

[[nodiscard]] std::string TcpServer::getPrivateKey() const {
    return private_key;
}

void TcpServer::setPrivateKey(const std::string& private_key) {
    this->private_key = private_key;
}

void TcpServer::handleCloseSocket(std::shared_ptr<ssl_socket> socket_ptr) {
    socket_ptr->async_shutdown(
        [self = shared_from_this(), socket_ptr](const asio::error_code& e) {
            if (e) {
                error(e.message());
            }
            socket_ptr->lowest_layer().close();
        });
}

void TcpServer::run() {
    asio::io_context::work work(*io);
    asio::thread_pool threadPool(threads);
    for (std::size_t i = 0; i < threads; ++i) {
        asio::post(threadPool, [self = shared_from_this()] {
            try {
                self->io->run();
            } catch (asio::system_error& e) {
                error(e.what());
            }
        });
    }
    threadPool.join();
}

void TcpServer::handleSignal(std::weak_ptr<ssl_socket> ptr) {
    if (ptr.expired()) {
        return;
    };
    auto socket_ptr = ptr.lock();
    sig->async_wait([socket_ptr, self = shared_from_this()](
                        const std::error_code& e, int signal_number) {
        switch (signal_number) {
            case SIGINT:
                info("SIGINT received, shutting down");
                self->handleCloseSocket(socket_ptr);
                self->io->stop();
                break;
            case SIGTERM:
                info("SIGTerm received, shutting down");
                self->handleCloseSocket(socket_ptr);
                self->io->stop();
                break;
            default:
                info("default {}", e.message());
        }
    });
}

auto TcpServer::handleFileAction(ProtoBuf& protoBuf)
    -> std::variant<std::string, std::vector<std::vector<char>>> {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    switch (method) {
        case ProtoBuf::Method::Query: {
            File file{path.string()};
            return file.QueryDirectory();
        }
        case ProtoBuf::Method::Get: {
            File file{path.string()};
            const auto& index = protoBuf.GetIndex();
            return file.GetFileDataSplited(index, filesplit);
        }
        case ProtoBuf::Method::Post: {
            fileWriteStrand->post([path, data] {
                File file{path.string() + ".sw"};
                file.SetFileData(data);
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
            File file{path.string()};
            file.DeleteActualFile();
            return "delete file OK";
        }
        default:
            throw std::runtime_error("unknown method");
    }
    return "unknown method";
}

void TcpServer::handleAccept() {
    asio::ssl::context ssl_context{asio::ssl::context::tls};
    ssl_context.set_options(asio::ssl::context::default_workarounds |
                            asio::ssl::context::no_sslv2);
    ssl_context.set_verify_mode(asio::ssl::verify_peer |
                                asio::ssl::verify_fail_if_no_peer_cert);
    ssl_context.set_verify_mode(1);
    ssl_context.use_certificate_file(certificate, asio::ssl::context::pem);
    ssl_context.use_private_key_file(private_key, asio::ssl::context::pem);

    auto socketPtr = std::make_shared<ssl_socket>(*io, ssl_context);
    auto writeSession = std::make_shared<WriteSession>(socketPtr, io);

    handleSignal(socketPtr);

    acceptor = std::make_unique<asio::ip::tcp::acceptor>(
        *io, asio::ip::tcp::endpoint(asio::ip::address::from_string(ip), port));

    info("waiting connection");
    acceptor->async_accept(
        socketPtr->lowest_layer(), [self = shared_from_this(), socketPtr,
                                    writeSession](const asio::error_code& e) {
            info("connection accepted");
            self->handleAccept();
            if (e) {
                error("async_accept: " + e.message());
            } else {
                socketPtr->async_handshake(
                    asio::ssl::stream_base::server,
                    [self, socketPtr, writeSession](const asio::error_code& e) {
                        info("handshake success");
                        writeSession->doWrite();
                        self->handleRead(socketPtr, writeSession);
                    });
            }
        });
}

void TcpServer::handleRead(std::shared_ptr<ssl_socket> socketPtr,
                           std::shared_ptr<WriteSession> writeSession) {
    debug("new read");
    auto streambuf = std::make_shared<asio::streambuf>();
    auto peek = std::make_shared<std::array<char, sizeof(std::size_t)>>();

    asio::async_read(
        *socketPtr, *streambuf,
        [peek, streambuf, socketPtr](const asio::system_error& e,
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
        [streambuf, socketPtr, writeSession, self = shared_from_this()](
            const asio::error_code& e, std::size_t size) {
            if (e) {
                self->handleCloseSocket(socketPtr);
                error("async_read: {}", e.message());
                return;
            }

            debug("read complete");
            self->handleRead(socketPtr, writeSession);

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
                writeSession->enqueue(send);
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
                    writeSession->enqueue(ret);
                }
            }
        });
}
