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

#include "Encoder.h"
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
#include "../libs/ImGuiNotify.hpp"

#include "2D renderer/Renderer.h"
#include "ActionManager.h"


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

    void draw_export_pop_up();
    void draw_save_popup();
    void draw_menu_bar();
    void draw_player();
    void draw_viewport();
    void draw_keyframe_edition_window();
    void draw_command_edition_window();

    void handle_input();

    void update();
    void update_waveform_viewer();
    void update_light_manager();

    // Audio player
    void play_audio();
    void stop_audio();

    // Callback
    void keyframe_creation_callback(int64_t sample);
    void keyframe_deletion_callback();
    // New sample: sample the keyframe has been displaced to
    void keyframe_drag_callback(int64_t delta);
    void keyframe_selection_callback(int64_t keyframe_uuid);
    void reset_selection_callback();
    void keyframe_unselection_callback(int64_t keyframe_uuid);

    void space_keyframes_evenly(const std::set<int64_t>& keyframes);

    void update_keyframes();

    void new_group(const std::string& name, const std::vector<size_t>& ids);

    void on_save();
    void on_save_as();
    void on_open_project();
    void on_export();
    void on_load_song();
    void on_export_video();

    void on_r();
    void on_ctrl_z();
    void on_ctrl_y();
    void on_ctrl_c();
    void on_ctrl_v();
    void on_ctrl_l();

    void save_project(const std::string& path);
    void load_project(const std::string& path);
    void export_project(const std::string& path);
    void load_song(const std::string& path);

    void update_dialogs();

    void copy_color(const Color& color);
    void copy_command(const Command& command);
    void copy_commands(const std::vector<Command>& commands);
    void copy_keyframes(const std::set<int64_t>& uuids);
    void paste_keyframes(int64_t first_samples);

    // ImGui components
    void color_picker(const char* label, Color& color);

    void start_export(const std::string& path);
    void export_frame();
    void update_export();


private:
    double target_application_framerate = 60;

    GLFWwindow* window;
    WaveformViewer waveform_viewer;
    AudioManager audio_manager;
    LightManager light_manager;

    Odin::Renderer renderer;

    int light_count = 12;
    std::vector<Group> groups;

    // Player state
    float playback_speed = 1.0f;
    int sample_rate = 41000;
    int sample_count = 0;

    AppState i_s;
    ActionManager action_manager;

    bool is_keyframe_edition_window_visible = true;
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

    std::unique_ptr<pfd::save_file> export_video_dialog;
    bool is_export_video_dialog_active = false;

    // Save / filename
    bool is_loaded_from_file = false;
    std::string filepath;
    bool should_draw_save_popup = false;

    // Copy / Paste system
    //--------------------
    Command copied_command;
    bool has_copied_command = false;

    std::vector<Command> copied_commands;
    bool has_copied_commands = false;

    Color copied_color;
    bool has_copied_color = false;

    std::vector<std::pair<Keyframe, std::vector<Command>>> copied_keyframes;
    bool has_copied_keyframes = false;

    // Video exporting system
    //-----------------------
    bool is_exporting = false;
    double export_framerate = 60.f;
    int current_frame = 0;
    int max_frame = 0;
    MP4Encoder *encoder = nullptr;
};



#endif //ELISEAPP_H
