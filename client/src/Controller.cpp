#include "Controller.h"

#include <memory>
#include <thread>

#include "Properties.h"

Controller::Controller() {
    loggerRegister = std::make_shared<LoggerRegister>();
    logger = spdlog::get("logger");
    io = std::make_shared<asio::io_context>();
    client = std::make_shared<app::TcpClient>(io);
    viewModule = std::make_shared<app::ViewModule>(client);
    view = std::make_shared<app::view>(std::move(viewModule));
}

void Controller::readProperties() {
    try {
        const auto& value = Properties::readProperties("config.json");
        auto client = view->GetViewModule()->getClient();
        if (value["ip"].isNull()) {
            logger->error("ip is null");
        } else {
            client->setIp(value["ip"].asString());
        }
        if (value["port"].isNull()) {
            logger->error("port is null");
        } else {
            client->setPort(value["port"].asLargestUInt());
        }
        if (value["domain"].isNull()) {
            logger->warn("domain is null");
        } else {
            client->setDomain(value["domain"].asString());
        }
        if (value["filesplit"].isNull()) {
            logger->warn("filesplit is null");
        } else {
            client->setFilesplit(value["filesplit"].asLargestUInt());
        }
        if (value["level"].isNull()) {
            logger->warn("level is null");
        } else {
            this->level = value["level"].asString();
            loggerRegister->setLevel(this->level);
        }
        if (value["threads"].isNull()) {
            logger->warn("threads is null");
        } else {
            std::size_t threads = value["threads"].asLargestUInt();
            if (threads > 1)
                this->threads = threads;
            else
                this->threads = std::thread::hardware_concurrency();
        }
        if (value["font"].isNull()) {
            this->font = value["font"].asString();
            logger->info("ip: {} port: {} level: {} filesplit: {}",
                         client->getIp(), client->getPort(), level,
                         client->getFilesplitsize());
        }
    } catch (std::exception& e) {
        logger->warn("{}", e.what());
    }
}

void Controller::init() {
    view->GetViewModule()->getClient()->connect();
    view->init(shared_from_this(), font);
    view->loop();
}

void Controller::run() {
    std::thread([this]() {
        asio::thread_pool threadPool(threads);
        for (int i = 0; i < threads - 1; i++) {
            asio::post(threadPool, [this]() { io->run(); });
        }
        threadPool.join();
    }).detach();
}

void Controller::stop() {
    view->GetViewModule()->getClient()->disconnect();
    io->stop();
    logger->info("io_context stop");
}
