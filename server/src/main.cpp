#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <memory>

#include "LoggerRegister.h"
#include "Properties.h"
#include "TcpServer.h"

class Controller {
    std::shared_ptr<LoggerRegister> loggerRegister;
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<TcpServer> server;

   public:
    Controller() {
        loggerRegister = std::make_shared<LoggerRegister>();
        logger = spdlog::get("logger");
        server = std::make_shared<TcpServer>();
    }
    void readProperties() {
        try {
            const auto value = Properties::readProperties("config.json");
            if (value["ip"].isNull()) {
                logger->error("ip is null");
            } else {
                server->setIp(value["ip"].asString());
            }
            if (value["port"].isNull()) {
                logger->error("port is null");
            } else {
                server->setPort(value["port"].asUInt());
            }
            if (value["log"].isNull()) {
                logger->warn("log is null");
            } else {
                loggerRegister->setLevel(value["log"].asString());
            }
            if (value["filesplit"].isNull()) {
                logger->warn("filesplit is null");
            } else {
                server->setFilesplit(value["filesplit"].asUInt64());
            }
            if (value["threads"].isNull()) {
                logger->warn("threads is null");
            } else {
                server->setThreads(value["threads"].asUInt());
            }
            if (value["certificate"].isNull()) {
                logger->error("certificate is null");
            } else {
                server->setCertificate(value["certificate"].asString());
            }
            if (value["privatekey"].isNull()) {
                logger->error("privatekey is null");
            } else {
                server->setPrivateKey(value["privatekey"].asString());
            }
            logger->info(
                "ip:{} port:{} filesplit:{} threads:{} certificate: "
                "{} privatekey: {}",
                server->getIp(), server->getPort(), server->getFilesplitsize(),
                server->getThreads(), server->getCertificate(),
                server->getPrivateKey());
        } catch (std::exception& e) {
            logger->warn("{}", e.what());
        }
    }

    void run() {
        server->handleAccept();
        server->run();
    }
};

int main(int argc, char* argv[]) {
    Controller controller;
    controller.readProperties();
    controller.run();
    return 0;
}
