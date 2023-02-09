#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "File.h"
#include "ProtoBuf.h"
#include "asio/error_code.hpp"

using namespace spdlog;
TcpServer::TcpServer(asio::ip::tcp::endpoint ep, size_t thread_pool_size)
    : ep(std::move(ep)),
      acceptor(io, std::move(ep)),
      threadPool(thread_pool_size) {}

void TcpServer::Process() {
    set_level(spdlog::level::debug);

    debug("Waiting for connection...");

    handleAccept();

    io.run();
}

std::string TcpServer::handleFileAction(ProtoBuf& protoBuf) {
    auto method = protoBuf.GetMethod();
    auto path = protoBuf.GetPath();
    auto data = protoBuf.GetData();

    std::string result;
    File file;
    file.SetFilePath(path);

    debug("method: {}", ProtoBuf::MethodToString(method));
    debug("path: {}", path.string());
    debug("data: {}", data);

    switch (method) {
        case ProtoBuf::Method::Get: {
            result = file.QueryDirectory();
            break;
        }
        case ProtoBuf::Method::Post: {
            auto data = protoBuf.GetData();
            debug("data: {}", data);
            file.SetFileData(data);
            result = "add file OK";
            break;
        }
        case ProtoBuf::Method::Delete:
            file.DeleteActualFile();
            result = "delete file OK";
            break;
        default:
            break;
    }
    return result;
}

void TcpServer::handleResult(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                             std::string result) {
    socket_ptr->write_some(asio::buffer(result));
    debug("send result to client");
    debug("{}", result);
}

void TcpServer::handleSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                             const asio::error_code e) {
    if (e) {
        error("Error: {}", e.message());
        return;
    }
    debug("Connection accepted");

    asio::streambuf streambuf;
    asio::read_until(*socket_ptr, streambuf, '\n');

    ProtoBuf protoBuf;
    std::istream is(&streambuf);
    is >> protoBuf;

    handleResult(socket_ptr, handleFileAction(protoBuf));
}

void TcpServer::handleAccept() {
    try {
        std::shared_ptr<asio::ip::tcp::socket> socket_ptr(
            new asio::ip::tcp::socket(io));
        acceptor.async_accept(*socket_ptr,
                              [this, socket_ptr](asio::error_code e) {
                                  handleSocket(socket_ptr, e);
                                  handleAccept();
                              });
    } catch (asio::system_error& e) {
        // TODO: error handling

        if (e.code() == asio::error::eof ||
            e.code() == asio::error::connection_reset) {
            // socket_ptr->close();
            debug("Connection closed with {}", e.what());
        } else {
            std::string tmp(e.what());
            // socket_ptr->write_some(asio::buffer(tmp));
            debug("send {} to client", e.what());
            debug("Connection closed");
        }
    }
}
