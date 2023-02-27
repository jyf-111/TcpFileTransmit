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
        controller->readProperties();
        controller->run();
        controller->initView();
    } catch (std::exception& e) {
        error("{}", e.what());
    }
    return 0;
}
