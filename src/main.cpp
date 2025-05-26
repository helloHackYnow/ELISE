#include "EliseApp.h"

#define MINIAUDIO_IMPLEMENTATION
#include "../libs/miniaudio.h"

int main() {

    EliseApp app;
    if (app.init()) {
        app.mainloop();
        app.cleanup();
    }
    return 0;
}