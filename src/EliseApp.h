//
// Created by victor on 26/05/25.
//

#ifndef ELISEAPP_H
#define ELISEAPP_H
#define GLFW_INCLUDE_NONE
#include <iostream>
#include <map>

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "../libs/tinyfiledialogs.h"
#include "WaveformViewer.h"
#include "AudioManager.h"
#include <GLFW/glfw3.h>
#include "Viewport.h"
#include "LightManager.h"
#include "ImGui_themes.h"
#include "JsonHandler.h"
#include "Exporter.h"


class EliseApp {

public:
    EliseApp();
    bool init();
    void mainloop();
    void cleanup();

private:

    void init_groups();
    void init_light_manager();
    void compile_commands();

    void draw();

    void draw_menu_bar();
    void draw_player();
    void draw_viewport();
    void draw_keyframe_edition_window();
    void draw_command_edition_window();

    void handle_input();

    void update();
    void update_waveform_viewer();
    void update_light_manager();
    void update_viewport();

    // Audio player
    void play_audio();
    void stop_audio();

    // Keyframes handling
    void order_keyframes();
    void key_frame_creation_callback(float sample);
    void key_frame_deletion_callback(int keyframe_index);

    // New sample: sample the keyframe has been displaced to
    void key_frame_drag_callback(int keyframe_index, int64_t new_sample);
    void key_frame_selection_callback(int keyframe_index);

    void update_keyframes();

    void new_group(const std::string& name, const std::vector<size_t>& ids);

    void on_save();
    void on_load();
    void on_export();

    void save_project(const std::string& path);
    void load_project(const std::string& path);


private:
    GLFWwindow* window;
    WaveformViewer waveform_viewer;
    AudioManager audio_manager;
    Viewport viewport;

    LightManager light_manager;

    int light_count = 12;
    std::vector<Group> groups;

    // Player state
    float playback_speed = 1.0f;

    // Keyframes
    std::map<int, Keyframe> keyframes_map {};
    std::vector<Keyframe> keyframes;
    int selected_keyframe = -1;
    bool is_keyframe_edition_window_visible = false;

    // Commands
    int selected_command = -1;
    bool is_command_edition_window_visible = false;

    // Project manager state
    std::string project_path;
    bool is_project_manager_visible = false;
};



#endif //ELISEAPP_H
