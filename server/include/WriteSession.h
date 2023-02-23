#pragma once

#include <asio.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

#include "ProtoBuf.h"
#include "spdlog/spdlog.h"

class WriteSession : public std::enable_shared_from_this<WriteSession> {
    std::shared_ptr<asio::ip::tcp::socket> socketPtr;
    std::queue<ProtoBuf> writeQueue;
    std::mutex mtx;

   public:
    WriteSession(std::shared_ptr<asio::ip::tcp::socket> socketPtr)
        : socketPtr(std::move(socketPtr)) {}

    void enqueue(const ProtoBuf& buf) {
        std::lock_guard<std::mutex> lock(mtx);
        writeQueue.push(buf);
        info("size = {}", writeQueue.size());
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
                              std::stringstream ss;
                              ss << std::this_thread::get_id();
                              debug("threadid = {} ;write: {} byte", ss.str(),
                                    size);
                              self->doWrite();
                          });
    }
};
