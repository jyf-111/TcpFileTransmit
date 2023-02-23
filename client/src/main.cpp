#include <asio.hpp>
#include <memory>
#include <thread>

#include "Properties.h"
#include "TcpClient.h"
#include "ViewModule.h"
#include "view.h"

class Controller : public std::enable_shared_from_this<Controller> {
    std::string level;
    std::size_t threads;
    std::shared_ptr<asio::io_context> io;
    std::shared_ptr<app::TcpClient> client;
    std::shared_ptr<app::ViewModule> viewModule;
    std::shared_ptr<app::view> view;

   public:
    Controller() {
        io = std::make_shared<asio::io_context>();
        client = std::make_shared<app::TcpClient>(io);
        viewModule = std::make_shared<app::ViewModule>(client);
        view = std::make_shared<app::view>(viewModule);
    }

    void setLevel(const std::string& level) {
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

    void readProperties() {
        try {
            Properties properties;
            auto value = properties.readProperties();
            auto client = view->GetViewModule()->getClient();
            client->setIp(value["ip"].asString());
            client->setPort(value["port"].asLargestUInt());
            client->setDomain(value["domain"].asString());
            client->setFilesplit(value["splitsize"].asLargestUInt());

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

    void init() {
        view->GetViewModule()->getClient()->connect();
        view->init(shared_from_this());
        view->loop();
    }

    void run() {
        std::thread([this]() {
            asio::thread_pool threadPool(threads);
            for (int i = 0; i < threads - 1; i++) {
                asio::post(threadPool, [this]() { io->run(); });
            }
            threadPool.join();
        }).detach();
    }
};

int main(int, char**) {
#ifdef _WIN32
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }
#endif
    try {
        auto controller = std::make_shared<Controller>();
        controller->readProperties();
        controller->run();
        controller->init();
    } catch (std::exception& e) {
        error("{}", e.what());
    }
    return 0;
}
