//
// Created by victor on 26/05/25.
//

#ifndef ELISEAPP_H
#define ELISEAPP_H
#define GLFW_INCLUDE_NONE
#include <iostream>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../libs/tinyfiledialogs.h"
#include "WaveformViewer.h"
#include "AudioManager.h"
#include <GLFW/glfw3.h>
#include "Viewport.h"


class EliseApp {

public:
    EliseApp();
    bool init();
    void mainloop();
    void cleanup();

private:
    void draw();

    void draw_menu_bar();
    void draw_project_manager();
    void draw_player();
    void draw_viewport();

    void handle_input();

    void update();
    void update_waveform_viewer();

    // Audio player
    void play_audio();
    void stop_audio();

private:
    GLFWwindow* window;
    WaveformViewer waveform_viewer;
    AudioManager audio_manager;
    Viewport viewport;

    // Player state
    float playback_speed = 1.0f;



    // Project manager state
    std::string project_path;
    bool is_project_manager_visible = false;
};



#endif //ELISEAPP_H
