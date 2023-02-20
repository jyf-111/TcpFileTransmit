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
            view->GetViewModule()->client->setIp(value["ip"].asString());
            view->GetViewModule()->client->setPort(value["port"].asUInt());
            view->GetViewModule()->client->setDomain(
                value["domain"].asString());
            view->GetViewModule()->client->setLevel(value["log"].asString());
            view->GetViewModule()->client->setFilesplit(
                value["splitsize"].asUInt());
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
    auto view = std::make_unique<app::view>();

    Controller controller(std::move(view));
    controller.readProperties();
    controller.run();
    return 0;
}
