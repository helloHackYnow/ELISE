#define NOMINMAX
#include "EliseApp.h"



#define MINIAUDIO_IMPLEMENTATION
#include "../libs/miniaudio.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"
int main() {

    EliseApp app;
    if (app.init()) {
        app.mainloop();
        app.cleanup();
    }
    return 0;
}