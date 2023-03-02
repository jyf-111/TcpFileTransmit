#include "Controller.h"

Controller::Controller() {
    loggerRegister = std::make_shared<LoggerRegister>();
    logger = spdlog::get("logger");
    assert(logger != nullptr);

    server = std::make_shared<TcpServer>();
}

void Controller::run() {
    server->init();
    server->handleAccept();
    server->run();
}
