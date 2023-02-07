#include <asio.hpp>

#include "TcpServer.h"
#include "def.h"

int main(int argc, char *argv[]) {
    TcpServer server(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), PORT),
                     THREAD_POOL_SIZE);
    server.Process();

    return 0;
}
