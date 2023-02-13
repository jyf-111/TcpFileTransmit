#include <iostream>

#include "ViewModule.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"

namespace app {
class view {
    app::ViewModule clientModule;
    GLFWwindow* window;

    // windows status
    bool show_demo_window = false;
    bool show_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

   public:
    void init();
    void loop();
    void connect();
    static void glfw_error_callback(int error, const char* description);
    ~view();
};

}  // namespace app
