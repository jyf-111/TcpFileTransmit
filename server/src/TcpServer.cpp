#include "TcpServer.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <string>

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
            result += "\n";
            break;
        }
        case ProtoBuf::Method::Post: {
            auto data = protoBuf.GetData();
            file.SetFileData(data);
            result = "add file OK\n";
            break;
        }
        case ProtoBuf::Method::Delete:
            file.DeleteActualFile();
            result = "delete file OK\n";
            break;
        default:
            throw std::runtime_error("unknown method");
    }
    return result;
}

void TcpServer::handleSocket(std::shared_ptr<asio::ip::tcp::socket> socket_ptr,
                             const asio::error_code& e) {
    if (e) {
        error("Error: {}", e.message());
        return;
    }
    debug("Connection accepted");

    handleReadWrite(socket_ptr);
}

void TcpServer::handleAccept() {
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr =
        std::make_shared<asio::ip::tcp::socket>(io);

    acceptor.async_accept(*socket_ptr,
                          [this, socket_ptr](const asio::error_code& e) {
                              if (e) {
                                  error("Error: {}", e.message());
                                  return;
                              }
                              handleSocket(socket_ptr, e);
                              handleAccept();
                          });
    try {
        io.reset();
        io.run();
    } catch (asio::system_error& e) {
        error("Error: {}", e.what());
    }
}

void TcpServer::handleReadWrite(
    std::shared_ptr<asio::ip::tcp::socket> socket_ptr) {
    // NOTE: this is a hack to set the socket to timeout
    socket_ptr->set_option(asio::socket_base::keep_alive(true));
    // the timeout value
    unsigned int timeout_milli = 1000;

// platform-specific switch
#if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || \
    defined WIN64 || defined WINNT
    // use windows-specific time
    int32_t timeout = timeout_milli;
    setsockopt(socket_ptr->native_handle(), SOL_SOCKET, SO_RCVTIMEO,
               (const char*)&timeout, sizeof(timeout));
    setsockopt(socket_ptr->native_handle(), SOL_SOCKET, SO_SNDTIMEO,
               (const char*)&timeout, sizeof(timeout));
#else
    // assume everything else is posix
    struct timeval tv;
    tv.tv_sec = timeout_milli / 1000;
    tv.tv_usec = (timeout_milli % 1000) * 1000;
    setsockopt(socket_ptr->native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv,
               sizeof(tv));
    setsockopt(socket_ptr->native_handle(), SOL_SOCKET, SO_SNDTIMEO, &tv,
               sizeof(tv));
#endif

    std::shared_ptr<asio::streambuf> streambuf =
        std::make_shared<asio::streambuf>();
    debug("new handle read write");

    asio::async_read_until(
        *socket_ptr, *streambuf, '\n',
        [streambuf, socket_ptr, this](const asio::error_code& e, size_t size) {
            if (e) {
                socket_ptr->close();
                error(e.message());
                return;
            }
            ProtoBuf protoBuf;
            std::istream is(streambuf.get());
            is >> protoBuf;

            std::shared_ptr<std::string> result =
                std::make_shared<std::string>(handleFileAction(protoBuf));

            asio::async_write(*socket_ptr, asio::buffer(*result),
                              [this, socket_ptr, result](
                                  const asio::error_code& e, size_t size) {
                                  if (e) {
                                      socket_ptr->close();
                                      info("socket close");
                                      error(e.message());
                                      return;
                                  }
                                  debug("send result to client success");
                                  handleReadWrite(socket_ptr);
                              });
        });
    try {
        io.reset();
        io.run();
    } catch (const asio::system_error& e) {
        error(e.what());
    }
}
