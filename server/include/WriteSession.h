#pragma once

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>

#include "ProtoBuf.h"

using namespace spdlog;

class WriteSession : public std::enable_shared_from_this<WriteSession> {
    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<ssl_socket> socketPtr;
    std::queue<ProtoBuf> writeQueue;
    std::shared_ptr<asio::steady_timer> timer;
    std::mutex mtx;

   public:
    WriteSession(std::shared_ptr<ssl_socket> socketPtr,
                 std::shared_ptr<asio::io_context> io)
        : timer(std::make_shared<asio::steady_timer>(*io,
                                                     std::chrono::seconds(3))),
          socketPtr(std::move(socketPtr)) {}

    void enqueue(const ProtoBuf& buf) {
        std::lock_guard<std::mutex> lock(mtx);
        writeQueue.push(buf);
    }

    void doWrite() {
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
};
