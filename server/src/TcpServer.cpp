#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <string>

#include "File.h"
#include "ProtoBuf.h"

using namespace spdlog;

TcpServer::TcpServer(asio::ip::tcp::endpoint ep, size_t thread_pool_size)
    : ep(std::move(ep)), acceptor(io, std::move(ep)) {
    set_level(spdlog::level::debug);
}

void TcpServer::run() {
    handleAccept();
    try {
        io.run();
    } catch (asio::system_error& e) {
        error("Error: {}", e.what());
    }
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
            result += "\n";
            break;
        }
        case ProtoBuf::Method::Post: {
            auto data = protoBuf.GetData();
            file.SetFileData(data);
            result = "add_file_OK\n";
            break;
        }
        case ProtoBuf::Method::Delete:
            file.DeleteActualFile();
            result = "delete_file_OK\n";
            break;
        default:
            throw std::runtime_error("unknown method");
    }
    return result;
}

void TcpServer::handleAccept() {
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr =
        std::make_shared<asio::ip::tcp::socket>(io);

    debug("connecting");
    acceptor.async_accept(
        *socket_ptr, [this, socket_ptr](const asio::error_code& e) {
            if (e) {
                socket_ptr->shutdown(asio::ip::tcp::socket::shutdown_both);
                socket_ptr->close();

                info("socket close");
                error("connect Error: {}", e.message());
            } else {
                handleReadWrite(socket_ptr);
                debug("Connection accepted");
            }
            handleAccept();
        });
}

void TcpServer::handleReadWrite(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    std::shared_ptr<asio::streambuf> streambuf =
        std::make_shared<asio::streambuf>();
    debug("new handle read write");

    asio::async_read_until(
        *socket_ptr, *streambuf, '\n',
        [streambuf, socket_ptr, this](const asio::error_code& e, size_t size) {
            if (e) {
                socket_ptr->shutdown(asio::ip::tcp::socket::shutdown_both);
                socket_ptr->close();
                info("socket close");
                error(e.message());
                return;
            }
            ProtoBuf protoBuf;
            std::shared_ptr<std::string> result;
            try {
                std::istream is(streambuf.get());
                is >> protoBuf;
                result =
                    std::make_shared<std::string>(handleFileAction(protoBuf));
            } catch (std::filesystem::filesystem_error& e) {
                error(e.what());
                result = std::make_shared<std::string>("error_path\n");
            }

            asio::async_write(*socket_ptr, asio::buffer(*result),
                              [this, socket_ptr, result](
                                  const asio::error_code& e, size_t size) {
                                  if (e) {
                                      socket_ptr->shutdown(
                                          asio::ip::tcp::socket::shutdown_both);
                                      socket_ptr->close();
                                      info("socket close");
                                      error(e.message());
                                      return;
                                  }
                                  debug("send result to client success");
                                  handleReadWrite(socket_ptr);
                              });
        });
}
