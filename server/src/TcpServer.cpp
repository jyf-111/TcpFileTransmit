#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"

using namespace spdlog;
TcpServer::TcpServer(asio::ip::tcp::endpoint ep, size_t thread_pool_size)
    : ep(std::move(ep)),
      acceptor(io, std::move(ep)),
      threadPool(thread_pool_size) {}

void TcpServer::Process() {
    set_level(spdlog::level::debug);

    debug("Waiting for connection...");

    handleAccept();
}

std::string TcpServer::handleFileAction(ProtoBuf& protoBuf) {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    std::string result;
    File file(path);

    debug("method: {} path: {} data: {}", ProtoBuf::MethodToString(method),
          path.string(), data);

    switch (method) {
        case ProtoBuf::Method::Get: {
            result = file.QueryDirectory();
            break;
        }
        case ProtoBuf::Method::Post: {
            auto data = protoBuf.GetData();
            file.SetFileData(data);
            result = "add file OK";
            break;
        }
        case ProtoBuf::Method::Delete:
            file.DeleteActualFile();
            result = "delete file OK";
            break;
        default:
            throw std::runtime_error("unknown method");
    }
    return result + ';';
}

void TcpServer::handleResult(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                             const std::string result) {
    asio::async_write(*socket_ptr, asio::buffer(result),
                      [&result](const asio::error_code& e, size_t size) {
                          if (e) throw e;
                          debug("send result to client");
                          debug("{}", result);
                      });
}

void TcpServer::handleSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                             const asio::error_code& e) {
    if (e) throw e;
    debug("Connection accepted");

    handleReadWrite(socket_ptr);
}

void TcpServer::handleAccept() {
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr =
        std::make_shared<asio::ip::tcp::socket>(io);

    acceptor.async_accept(*socket_ptr,
                          [this, socket_ptr](const asio::error_code& e) {
                              if (e) throw e;
                              handleSocket(socket_ptr, e);
                              handleAccept();
                          });
    try {
        io.run();
    } catch (asio::system_error& e) {
        if (e.code() == asio::error::eof ||
            e.code() == asio::error::connection_reset) {
            socket_ptr->close();
            debug("Connection closed with {}", e.what());
        } else {
            std::string tmp(e.what());
            socket_ptr->write_some(asio::buffer(tmp));
            debug("send {} to client", e.what());
            debug("Connection closed");
        }
    }
}

void TcpServer::handleReadWrite(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    std::shared_ptr<asio::streambuf> streambuf =
        std::make_shared<asio::streambuf>();
    asio::async_read_until(
        *socket_ptr, *streambuf, '\n',
        [streambuf, socket_ptr, this](const asio::error_code& e, size_t size) {
            if (e) throw e;
            ProtoBuf protoBuf;
            std::istream is(streambuf.get());
            is >> protoBuf;

            handleResult(socket_ptr, handleFileAction(protoBuf));
            handleReadWrite(socket_ptr);
        });
    try {
        io.run();
    } catch (const asio::system_error& e) {
        error("Error: {}", e.what());
    }
}
