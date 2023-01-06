#include <asio.hpp>

#include "TcpServer.h"

int main(int argc, char *argv[]) {

    int port = 1234;
    size_t threadPoolSize = 4;
    TcpServer server(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port),
                     threadPoolSize);

    return 0;
}
