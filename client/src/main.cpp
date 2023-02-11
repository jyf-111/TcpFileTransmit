#include "view.h"

int main(int, char**) {
    app::view view;
    view.init();
    view.connect();
    view.loop();

    return 0;
}
