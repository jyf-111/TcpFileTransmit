#pragma once

#include <asio.hpp>
#include <filesystem>

#include "ThreadPool.h"

class TcpServer {
    asio::io_service io;
    asio::ip::tcp::endpoint ep /* (asio::ip::tcp::v4(), 1234); */;
    asio::ip::tcp::acceptor acceptor /*(io, ep)*/;
    // NOTE:thread pool
    ThreadPool threadPool;

   public:
    TcpServer() = delete;
    TcpServer(asio::ip::tcp::endpoint, size_t);
    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;
};
