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
        client->setIp(value["ip"].asString());
        client->setPort(value["port"].asLargestUInt());
        client->setDomain(value["domain"].asString());
        client->setFilesplit(value["filesplit"].asLargestUInt());
        loggerRegister->setLevel(level = value["log"].asString());
        logger->info("ip: {} port: {} level: {} filesplit: {}", client->getIp(),
                     client->getPort(), level, client->getFilesplitsize());

        std::size_t threads = value["threads"].asLargestUInt();
        if (threads > 1)
            this->threads = threads;
        else
            this->threads = std::thread::hardware_concurrency();

        this->font = value["font"].asString();

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
