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

#include "WaveformViewer.h"
#include "AudioManager.h"
#include <GLFW/glfw3.h>
#include "Viewport.h"
#include "LightManager.h"
#include "ImGui_themes.h"
#include "JsonHandler.h"
#include "Exporter.h"
#include "../libs/portable_file_dialog.h"
#include "file_utils.h"

#include "../libs/stb_image.h"
#include "../icons/logo32.h"
#include "../icons/logo128.h"

#include "../resources/SourceCodePro_Semibold.h"
#include "../resources/MaterialSymbols.h"

class EliseApp {

public:
    EliseApp();
    bool init();
    void mainloop();
    void cleanup();

private:

	void configure_dockspace();

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
    void build_keyframe_uuid_to_index_map();


    // Callback
    void key_frame_creation_callback(int64_t sample);
    void key_frame_deletion_callback(int64_t keyframe_uuid);
    // New sample: sample the keyframe has been displaced to
    void key_frame_drag_callback(int64_t keyframe_uuid, int64_t new_sample);
    void key_frame_selection_callback(int64_t keyframe_uuid);

    void update_keyframes();

    void new_group(const std::string& name, const std::vector<size_t>& ids);

    void on_save();
    void on_open_project();
    void on_export();
    void on_load_song();

    void save_project(const std::string& path);
    void load_project(const std::string& path);
    void export_project(const std::string& path);
    void load_song(const std::string& path);

    void update_dialogs();


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
    std::vector<Keyframe> keyframes;
    std::map<int64_t, int> keyframe_uuid_to_index;
    int64_t selected_keyframe_uuid = -1;
    int64_t max_keyframe_uuid = 0;
    bool is_keyframe_edition_window_visible = true;

    // Commands
    std::unordered_map<int64_t, std::vector<Command>> keyframe_uuid_to_commands;
    int selected_command = -1;
    bool is_command_edition_window_visible = false;

    // Project manager state
    std::string project_path;
    bool is_project_manager_visible = false;


    // Threaded File Dialog
    bool is_dialog_opened = false;

    std::unique_ptr<pfd::open_file> open_project_dialog;
    bool is_open_project_dialog_active = false;

    std::unique_ptr<pfd::open_file> load_song_dialog;
    bool is_load_song_dialog_active = false;

    std::unique_ptr<pfd::save_file> save_project_dialog;
    bool is_save_project_dialog_active = false;

    std::unique_ptr<pfd::save_file> export_project_dialog;
    bool is_export_project_dialog_active = false;

};



#endif //ELISEAPP_H
