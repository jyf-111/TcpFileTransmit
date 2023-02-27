#include "Controller.h"

#include <thread>

Controller::Controller() {
    io = std::make_shared<asio::io_context>();
    client = std::make_shared<app::TcpClient>(io);
    viewModule = std::make_shared<app::ViewModule>(client);
    view = std::make_shared<app::view>(viewModule);
}

void Controller::setLevel(const std::string& level) {
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
    } else {
        set_level(spdlog::level::info);
    }
}

void Controller::readProperties() {
    try {
        Properties properties;
        auto value = properties.readProperties();
        auto client = view->GetViewModule()->getClient();
        client->setIp(value["ip"].asString());
        client->setPort(value["port"].asLargestUInt());
        client->setDomain(value["domain"].asString());
        client->setFilesplit(value["filesplit"].asLargestUInt());

        level = value["log"].asString();
        setLevel(level);

        std::size_t threads = value["threads"].asLargestUInt();
        if (threads > 1)
            this->threads = threads;
        else
            this->threads = std::thread::hardware_concurrency();

        info("ip: {} port: {} level: {} filesplit: {}", client->getIp(),
             client->getPort(), level, client->getFilesplitsize());
    } catch (std::exception& e) {
        warn("{}", e.what());
    }
}

void Controller::initView() {
    view->GetViewModule()->getClient()->connect();
    view->init(shared_from_this());
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
    debug("io_context stop");
}
