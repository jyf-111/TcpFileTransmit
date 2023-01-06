#pragma once
#include <asio.hpp>

class TcpServer {
    asio::io_service io;
    asio::ip::tcp::endpoint ep /* (asio::ip::tcp::v4(), 1234); */;
    asio::ip::tcp::acceptor acceptor /*(io, ep)*/;
	
   public:
    TcpServer(asio::ip::tcp::endpoint);
    TcpServer(const TcpServer &) = delete;
    TcpServer(TcpServer &&) = delete;
    TcpServer &operator=(const TcpServer &) = delete;
    TcpServer &operator=(TcpServer &&) = delete;
};
