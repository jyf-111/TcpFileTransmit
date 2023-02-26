#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

#include "ProtoBuf.h"
#include "spdlog/spdlog.h"

class WriteSession : public std::enable_shared_from_this<WriteSession> {
    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;
    std::shared_ptr<ssl_socket> socketPtr;
    std::queue<ProtoBuf> writeQueue;
    std::mutex mtx;

   public:
    WriteSession(std::shared_ptr<ssl_socket> socketPtr)
        : socketPtr(std::move(socketPtr)) {}

    void enqueue(const ProtoBuf& buf) {
        std::lock_guard<std::mutex> lock(mtx);
        writeQueue.push(buf);
    }

    void doWrite() {
        std::lock_guard<std::mutex> lock(mtx);
        if (writeQueue.empty()) return;

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