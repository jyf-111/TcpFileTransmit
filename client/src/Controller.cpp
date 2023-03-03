#include "Controller.h"

#include <memory>
#include <thread>

#include "Properties.h"

Controller::Controller() {
    loggerRegister = std::make_shared<LoggerRegister>();
    logger = spdlog::get("logger");
    assert(logger != nullptr);

    io = std::make_shared<asio::io_context>();
    client = std::make_shared<app::TcpClient>(io);
    viewModule = std::make_shared<app::ViewModule>(client);
    view = std::make_shared<app::view>(std::move(viewModule));
}

void Controller::init() {
    const auto& value = Properties::readProperties();
    this->level = value["level"].asString();
    loggerRegister->setLevel(level);

    this->threads = value["threads"].asLargestUInt();
    if (threads == 0) this->threads = std::thread::hardware_concurrency() - 1;
    this->font = value["font"].asString();

    logger->info("level: {} threads: {} font: {}", level, threads, font);

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
