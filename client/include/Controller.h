#pragma once

#include "Properties.h"
#include "TcpClient.h"
#include "ViewModule.h"
#include "view.h"

class Controller : public std::enable_shared_from_this<Controller> {
    std::size_t threads;
    std::shared_ptr<asio::io_context> io;
    std::shared_ptr<app::TcpClient> client;
    std::shared_ptr<app::ViewModule> viewModule;
    std::shared_ptr<app::view> view;

    void setLevel(const std::string& level);

   public:
    Controller();
    void readProperties();
    void initView();
    void run();
    void stop();
};
