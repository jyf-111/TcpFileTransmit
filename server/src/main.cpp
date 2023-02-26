#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <memory>

#include "Properties.h"
#include "TcpServer.h"

using namespace spdlog;

class Controller {
    std::shared_ptr<TcpServer> server;

   public:
    Controller(std::shared_ptr<TcpServer> server) : server(std::move(server)) {}
    void readProperties() {
        try {
            Properties properties;
            auto value = properties.readProperties();
            if (value["ip"].isNull()) {
                error("ip is null");
            } else {
                server->setIp(value["ip"].asString());
            }
            if (value["port"].isNull()) {
                error("port is null");
            } else {
                server->setPort(value["port"].asUInt());
            }
            if (value["log"].isNull()) {
                warn("log is null");
            } else {
                server->setLevel(value["log"].asString());
            }
            if (value["filesplit"].isNull()) {
                warn("filesplit is null");
            } else {
                server->setFilesplit(value["filesplit"].asUInt64());
            }
            if (value["threads"].isNull()) {
                warn("threads is null");
            } else {
                server->setThreads(value["threads"].asUInt());
            }
            if (value["certificate"].isNull()) {
                error("certificate is null");
            } else {
                server->setCertificate(value["certificate"].asString());
            }
            if (value["privatekey"].isNull()) {
                error("privatekey is null");
            } else {
                server->setPrivateKey(value["privatekey"].asString());
            }
            info(
                "ip:{} port:{} level:{} filesplit:{} threads:{} certificate: "
                "{} privatekey: {}",
                server->getIp(), server->getPort(), server->getLevel(),
                server->getFilesplitsize(), server->getThreads(),
                server->getCertificate(), server->getPrivateKey());
        } catch (std::exception& e) {
            warn("{}", e.what());
        }
    }
    void run() {
        server->init();
        server->handleAccept();
        server->run();
    }
};

int main(int argc, char* argv[]) {
    auto server = std::make_shared<TcpServer>();

    Controller controller(std::move(server));
    controller.readProperties();
    controller.run();
    return 0;
}
