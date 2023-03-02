#pragma once

#include <cassert>
#include <memory>

#include "LoggerRegister.h"
#include "TcpServer.h"

class Controller {
    std::shared_ptr<LoggerRegister> loggerRegister;
    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<TcpServer> server;

   public:
    Controller();
    void init();
    void run();
};
