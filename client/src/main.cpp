#include <memory>

#include "Controller.h"

int main(int, char**) {
#ifdef _WIN32
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }
#endif

    try {
        auto controller = std::make_shared<Controller>();
        controller->run();
        controller->init();
    } catch (std::exception& e) {
        spdlog::get("logger")->error("{}", e.what());
    }
    return 0;
}
