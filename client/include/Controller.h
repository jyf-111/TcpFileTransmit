#pragma once

#include <memory>

#include "LoggerRegister.h"
#include "Properties.h"
#include "TcpClient.h"
#include "ViewModule.h"
#include "view.h"

class Controller : public std::enable_shared_from_this<Controller> {
    std::shared_ptr<LoggerRegister> loggerRegister;
    std::shared_ptr<spdlog::logger> logger;

    std::size_t threads;
    std::string level;
    std::shared_ptr<asio::io_context> io;
    std::shared_ptr<app::TcpClient> client;
    std::shared_ptr<app::ViewModule> viewModule;
    std::shared_ptr<app::view> view;

   public:
    Controller();
    void readProperties();
    void init();
    void run();
    void stop();
};
