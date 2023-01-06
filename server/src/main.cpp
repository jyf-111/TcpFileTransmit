#include <asio.hpp>

#include "TcpServer.h"

int main(int argc, char *argv[]) {
    TcpServer server(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 1234));

    return 0;
}
