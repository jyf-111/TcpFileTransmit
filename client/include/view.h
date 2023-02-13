#include <iostream>
#include <memory>

#include "ViewModule.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"

namespace app {
class view {
    std::shared_ptr<app::ViewModule> viewModule =
        std::make_shared<app::ViewModule>();
    GLFWwindow* window;

    // windows status
    bool show_demo_window = false;
    bool show_window = true;

   public:
    void init();
    void loop();
    void connect();
    static void glfw_error_callback(int error, const char* description);
    ~view();
};

}  // namespace app
