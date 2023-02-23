#include <memory>

#include "ViewModule.h"
#include "imgui_impl_glfw.h"

class Controller;

namespace app {

class view {
    std::shared_ptr<app::ViewModule> viewModule;
    GLFWwindow* window;

    std::weak_ptr<Controller> controller;
    // windows status
    bool show_demo_window = false;
    bool show_window = true;

   public:
    view(std::shared_ptr<app::ViewModule>);
    void init(std::weak_ptr<Controller>);
    void loop();
    std::shared_ptr<app::ViewModule> GetViewModule();
    static void glfw_error_callback(int error, const char* description);
    ~view();
};

}  // namespace app
