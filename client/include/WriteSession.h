#pragma once

#include <asio.hpp>
#include <memory>
#include <mutex>
#include <queue>

#include "ProtoBuf.h"
#include "spdlog/spdlog.h"

class WriteSession : public std::enable_shared_from_this<WriteSession> {
    std::shared_ptr<asio::ip::tcp::socket> socketPtr;
    std::queue<ProtoBuf> writeQueue;
    asio::streambuf buf;
    std::ostream os{&buf};
    std::mutex PushMtx;
    std::mutex PopMtx;

   public:
    WriteSession(std::shared_ptr<asio::ip::tcp::socket> socketPtr)
        : socketPtr(std::move(socketPtr)) {}

    void enqueue(const ProtoBuf& buf) {
        std::lock_guard<std::mutex> lock(PushMtx);
        writeQueue.push(buf);
    }

    const ProtoBuf dequeue() {
        std::lock_guard<std::mutex> lock(PopMtx);
        ProtoBuf tmp = writeQueue.front();
        writeQueue.pop();
        return tmp;
    }

    void doWrite() {
        if (writeQueue.empty()) return;
        os << dequeue();
        asio::async_write(*socketPtr, buf,
                          [self = shared_from_this()](const asio::error_code& e,
                                                      std::size_t size) {
                              if (e) error("async_write: {}", e.message());
                              debug("write: {} byte", size);
                              self->doWrite();
                          });
    }
};
