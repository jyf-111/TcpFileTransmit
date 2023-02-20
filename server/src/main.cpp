#include <spdlog/spdlog.h>

#include <algorithm>
#include <asio.hpp>

#include "Properties.h"
#include "TcpServer.h"

using namespace spdlog;

class Controller {
    std::unique_ptr<TcpServer> server;

   public:
    Controller(std::unique_ptr<TcpServer> server) : server(std::move(server)) {}
    void readProperties() {
        try {
            Properties properties;
            auto value = properties.readProperties();
            server->setIp(value["ip"].asString());
            server->setPort(value["port"].asUInt());
            server->setLevel(value["log"].asString());
            server->setFilesplit(value["splitsize"].asUInt64());
            server->setThreads(value["threads"].asUInt());
            info("ip:{} port:{} levvel:{} filesplit:{} threads:{}",
                 server->getIp(), server->getPort(), server->getLevel(),
                 server->getFilesplitsize(), server->getThreads());
        } catch (std::exception& e) {
            warn("{}", e.what());
        }
    }
    void run() {
        server->handleSignal();
        server->handleAccept();
        server->run();
    }
};

int main(int argc, char* argv[]) {
    auto server = std::make_unique<TcpServer>();

    Controller controller(std::move(server));
    controller.readProperties();
    controller.run();
    return 0;
}
