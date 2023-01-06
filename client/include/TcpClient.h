#include <spdlog/spdlog.h>

#include <asio.hpp>

#include "def.h"
/**
 * @brief TcpClient
 */

class TcpClient {
    asio::io_service io;
    asio::ip::tcp::endpoint ep /*(asio::ip::address::from_string("127.0.0.1"),
                                1234)*/
        ;
    asio::ip::tcp::socket sock /*(io)*/;

   public:
    TcpClient(std::string ip, size_t port);
};
