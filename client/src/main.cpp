#include <memory>

#include "Properties.h"
#include "view.h"

class Controller {
    std::unique_ptr<app::view> view;

   public:
    Controller(std::unique_ptr<app::view> view) : view(std::move(view)) {}

    void readProperties() {
        try {
            Properties properties;
            auto value = properties.readProperties();
            auto client = view->GetViewModule()->getClient();
            client->setIp(value["ip"].asString());
            client->setPort(value["port"].asUInt());
            client->setDomain(value["domain"].asString());
            client->setLevel(value["log"].asString());
            client->setFilesplit(value["splitsize"].asUInt());
            info("ip: {} port: {} level: {} filesplit: {}", client->getIp(),
                 client->getPort(), client->getLevel(),
                 client->getFilesplitsize());
        } catch (std::exception& e) {
            warn("{}", e.what());
        }
    }
    void run() {
        view->init();
        view->GetViewModule()->getClient()->run();
        view->GetViewModule()->getClient()->connect();
        view->loop();
    }
};

int main(int, char**) {
#ifdef _WIN32
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }
#endif

    auto view = std::make_unique<app::view>();
    Controller controller(std::move(view));
    controller.readProperties();
    controller.run();
    return 0;
}
