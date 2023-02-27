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
                setLevel(value["log"].asString());
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
                "ip:{} port:{} filesplit:{} threads:{} certificate: "
                "{} privatekey: {}",
                server->getIp(), server->getPort(), server->getFilesplitsize(),
                server->getThreads(), server->getCertificate(),
                server->getPrivateKey());
        } catch (std::exception& e) {
            warn("{}", e.what());
        }
    }

    void setLevel(const std::string& level) {
        if (level == "debug") {
            set_level(spdlog::level::debug);
        } else if (level == "info") {
            set_level(spdlog::level::info);
        } else if (level == "warn") {
            set_level(spdlog::level::warn);
        } else if (level == "err") {
            set_level(spdlog::level::err);
        } else if (level == "critical") {
            set_level(spdlog::level::critical);
        } else if (level == "off") {
            set_level(spdlog::level::off);
        }
    }

    void run() {
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
