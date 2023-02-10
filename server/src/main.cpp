#include <asio.hpp>

#include "TcpServer.h"

#define PORT 1234
#define THREAD_POOL_SIZE 4

int main(int argc, char *argv[]) {
    TcpServer server(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), PORT),
                     THREAD_POOL_SIZE);
    server.handleAccept();

    return 0;
}
