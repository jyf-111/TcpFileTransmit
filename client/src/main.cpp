#include "view.h"

int main(int, char**) {
    app::view view;
    view.init();
    view.GetViewModule()->connect();
    view.loop();

    return 0;
}
