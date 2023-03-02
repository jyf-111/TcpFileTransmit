#include "Controller.h"
#include "Properties.h"

Controller::Controller() {
    loggerRegister = std::make_shared<LoggerRegister>();
    logger = spdlog::get("logger");
    assert(logger != nullptr);

    server = std::make_shared<TcpServer>();
}

void Controller::init() {
    server->init();

    const auto& value = Properties::readProperties();
    loggerRegister->setLevel(value["level"].asString());
}


void Controller::run() {
    server->handleAccept();
    server->run();
}
