#include <spdlog/spdlog.h>
#include <vcruntime.h>

#include <asio.hpp>
#include <iostream>
#include <memory>

#include "Properties.h"
#include "TcpServer.h"

#define PORT 1234
#define THREAD_POOL_SIZE 4

using std::cout;

int main(int argc, char* argv[]) {
    TcpServer server;
    server.readProperties();
    server.handleSignal();
    server.handleAccept();
    server.run();

    return 0;
}
