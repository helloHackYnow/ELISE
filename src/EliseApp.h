//
// Created by victor on 26/05/25.
//

#ifndef ELISEAPP_H
#define ELISEAPP_H

#include <iostream>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "WaveformViewer.h"
#include "AudioManager.h"
#include <GLFW/glfw3.h>


class EliseApp {

public:
    EliseApp();
    bool init();
    void mainloop();
    void cleanup();

private:
    void draw();

private:
    GLFWwindow* window;
    WaveformViewer waveform_viewer;
    AudioManager audio_manager;
};



#endif //ELISEAPP_H
