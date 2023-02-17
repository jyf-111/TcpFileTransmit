#include <memory>

#include "ViewModule.h"
#include "imgui_impl_glfw.h"

namespace app {
class view {
    std::shared_ptr<app::ViewModule> viewModule =
        std::make_shared<app::ViewModule>();
    GLFWwindow* window;

    // windows status
    bool show_demo_window = true;
    bool show_window = true;

   public:
    void init();
    void loop();
    std::shared_ptr<app::ViewModule> GetViewModule();
    static void glfw_error_callback(int error, const char* description);
    ~view();
};

}  // namespace app
