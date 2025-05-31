//
// Created by victor on 26/05/25.
//

#include "EliseApp.h"

#include <filesystem>

#include "imgui_internal.h"


EliseApp::EliseApp() {
}

bool EliseApp::init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window
    window = glfwCreateWindow(1280, 720, "Event-Linked Illumination and Sound Engine", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwMaximizeWindow(window);
    glfwSwapInterval(1); // Enable vsync

    // Load icons
    GLFWimage icons[2];
    int channels;

    // Load 16x16 icon
    icons[0].pixels = stbi_load_from_memory(logo32_png, logo32_png_len,
                                           &icons[0].width, &icons[0].height,
                                           &channels, 4);

    // Load 32x32 icon
    icons[1].pixels = stbi_load_from_memory(logo128_png, logo128_png_len,
                                           &icons[1].width, &icons[1].height,
                                           &channels, 4);

    if (icons[0].pixels && icons[1].pixels) {
        glfwSetWindowIcon(window, 2, icons);
    }

    // Free both images
    if (icons[0].pixels) stbi_image_free(icons[0].pixels);
    if (icons[1].pixels) stbi_image_free(icons[1].pixels);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup ImGui style
    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 1;
    font_config.PixelSnapH = false;

    io.Fonts->AddFontFromFileTTF("./ressources/Roboto-Regular.ttf", 18.0f, &font_config);

    // Setup Dear ImGui style
    setBessDarkColors();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);


    viewport.init(500, 500);

    waveform_viewer.key_frame_creation_callback = [this](float arg){key_frame_creation_callback(arg);};
    waveform_viewer.key_frame_deletion_callback = [this](int arg){key_frame_deletion_callback(arg);};
    waveform_viewer.key_frame_drag_callback = [this](int arg, int64_t arg2){key_frame_drag_callback(arg, arg2);};
    waveform_viewer.key_frame_selection_callback = [this](int arg){key_frame_selection_callback(arg);};

    init_groups();
    init_light_manager();

    return true;
}

void EliseApp::mainloop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        update();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Enable docking
        ImGui::DockSpaceOverViewport();

        draw();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void EliseApp::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void EliseApp::init_groups() {

    char buff[128];

    for (size_t i = 0; i < light_count; ++i) {
        sprintf(buff, "Light %d", i + 1);
        new_group(buff, {i});
    }

    new_group("All", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    new_group("Up", {0, 1, 2, 3, 4});
    new_group("Vertical Mid", {5, 6});
    new_group("Down", {7, 8, 9, 10, 11});

    new_group("Left", {0, 1, 2, 5, 7, 8});
    new_group("Right", {3, 4, 6, 9, 10, 11});

    new_group("Horizontal Mid", {2, 3, 5, 6, 8, 9});

    new_group("Diag up left", {0, 1, 2, 3, 5, 7});
    new_group("Diag up right", {4, 6, 8, 9, 10, 11});

    new_group("Diag down left", {0, 1, 5, 7, 8, 9});
    new_group("Diag down right", {2, 3, 4, 6, 10, 11});
}

void EliseApp::init_light_manager() {
    for (int i = 0; i < light_count; ++i) {
        light_manager.addLight();
    }

    for (auto & group: groups) {
        auto & name = group.name;
        auto & ids = group.lights;

        light_manager.new_group(ids);
    }

}

void EliseApp::compile_commands() {
    std::vector<Command> commands;

    order_keyframes();

    for (auto & keyframe: keyframes) {
        for (auto & command: keyframe_uuid_to_commands[keyframe.uuid]) {

            retimeCommand(command, keyframe.trigger_sample);

            commands.push_back(command);
        }
    }

    std::reverse(commands.begin(), commands.end());

    light_manager.reset();
    light_manager.setCommandStack(commands);
}

void EliseApp::draw() {
    ImGui::BeginDisabled(is_dialog_opened);

    draw_menu_bar();
    draw_player();
    draw_keyframe_edition_window();
    draw_command_edition_window();
    waveform_viewer.draw();
    viewport.draw();

    ImGui::EndDisabled();
}

void EliseApp::draw_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) on_open_project();
            ImGui::Separator();
            if (ImGui::MenuItem("Save")) on_save();
            ImGui::Separator();
            if (ImGui::MenuItem("Export")) on_export();

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Project Manager", nullptr, &is_project_manager_visible);
            ImGui::MenuItem("Keyframe Edition", nullptr, &is_keyframe_edition_window_visible);
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EliseApp::draw_player() {
    if (ImGui::Begin("Audio player")) {

        ImGui::Text("Audio file");
        ImGui::Separator();
        if (ImGui::Button("Load audio")) on_load_song();

        ImGui::Spacing();
        ImGui::Text("Player");
        ImGui::Separator();
        if (ImGui::Button("Play")) {
            play_audio();
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            stop_audio();
        }

        ImGui::Spacing();
        ImGui::Text("Playback option");
        ImGui::Separator();

        if ( ImGui::DragFloat("Speed", &playback_speed, 0.01f, 0.1f, 10.0f) && audio_manager.isPlaying()) {
            stop_audio();
            play_audio();
        }

        ImGui::End();
    }
}

void EliseApp::draw_viewport() {
    viewport.draw();
}

void EliseApp::draw_keyframe_edition_window() {
    if (ImGui::Begin("Keyframe", &is_keyframe_edition_window_visible)) {
        ImGui::Text("Keyframe");

        ImGui::Separator();

        if (selected_keyframe_uuid >= 0) {
            std::vector<std::string> commands_str;
            std::vector<const char*> listbox_buff;

            auto& commands = keyframe_uuid_to_commands[selected_keyframe_uuid];

            for (auto & command: keyframe_uuid_to_commands[selected_keyframe_uuid]) {
                commands_str.push_back("Command on group " + groups[command.group_id].name);
                listbox_buff.push_back(commands_str.back().c_str());
            }

            ImGui::ListBox("###", &selected_command, listbox_buff.data(), listbox_buff.size(), 6);



            if (ImGui::Button("Add")) {
                commands.push_back(Command{});
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(selected_command < 0 || selected_command >= commands.size());
            if (ImGui::Button("Delete")) {
                commands.erase(commands.begin() + selected_command);
            }

            ImGui::EndDisabled();




        } else {
            ImGui::BeginDisabled();
            ImGui::Text("No keyframe selected");
            ImGui::EndDisabled();
        }


        ImGui::End();
    }
}

void EliseApp::draw_command_edition_window() {
    if (ImGui::Begin("Command", &is_command_edition_window_visible)) {
        if (selected_keyframe_uuid < 0
            || selected_command < 0
            || selected_command >= keyframe_uuid_to_commands[selected_keyframe_uuid].size()) {
            ImGui::BeginDisabled();
            ImGui::Text("No command selected");
            ImGui::EndDisabled();
        } else {
            auto & command = keyframe_uuid_to_commands[selected_keyframe_uuid][selected_command];

            ImGui::Text("Command %d", selected_command);

            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::BeginCombo("Group", groups[command.group_id].name.c_str())) {
                for (int n = 0; n < groups.size(); n++) {
                    const bool is_selected = (command.group_id == n);
                    if (ImGui::Selectable(groups[n].name.c_str(), is_selected))
                        command.group_id = n;

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::BeginCombo("Kind", AnimationKind_to_str(command.animation.kind)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(AnimationKind_str); n++)
                {
                    const bool is_selected = (command.animation.kind == AnimationKind_from_int[n]);
                    if (ImGui::Selectable(AnimationKind_str[n], is_selected))
                        command.animation.kind = AnimationKind_from_int[n];

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            ImGui::Text("Animation");
            ImGui::Separator();

            switch (command.animation.kind) {
                case AnimationKind::gradient : {

                    auto & gradient = command.animation.gradient;

                    if (ImGui::BeginCombo("Interpolation", GradientKind_to_str(gradient.kind))) {
                        for (int n = 0; n < IM_ARRAYSIZE(GradientKind_str); n++) {
                            bool is_selected = (gradient.kind == GradientKind_from_int[n]);

                            if (ImGui::Selectable(GradientKind_str[n], is_selected))
                                gradient.kind = GradientKind_from_int[n];

                            if (is_selected)
                                ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }

                    ImGui::DragInt("Duration (in sample)", &gradient.duration);

                    ImGui::Spacing();

                    ImGui::Text("Color gradient");
                    ImGui::Separator();
                    // Start color
                    float col[4] = {
                        gradient.start_color.r / 255.0f,
                        gradient.start_color.g / 255.0f,
                        gradient.start_color.b / 255.0f,
                        gradient.start_color.a / 255.0f
                    };
                    ImGui::ColorEdit4("Start color", col);
                    gradient.start_color = {int(col[0] * 255), int(col[1] * 255), int(col[2] * 255), int(col[3] * 255)};

                    // End color
                    col[0] = gradient.end_color.r / 255.0f;
                    col[1] = gradient.end_color.g / 255.0f;
                    col[2] = gradient.end_color.b / 255.0f;
                    col[3] = gradient.end_color.a / 255.0f;
                    ImGui::ColorEdit4("End  color", col);
                    gradient.end_color = {int(col[0] * 255), int(col[1] * 255), int(col[2] * 255), int(col[3] * 255)};


                    break;
                }

                case AnimationKind::toggle : {

                    auto & toggle = command.animation.toggle;

                    ImGui::Checkbox("Toggle on", &toggle.is_on);
                    ImGui::Spacing();

                    ImGui::BeginDisabled(!toggle.is_on);

                    ImGui::Text("Color");
                    ImGui::Separator();

                    float col[4] = {
                        toggle.color.r / 255.0f,
                        toggle.color.g / 255.0f,
                        toggle.color.b / 255.0f,
                        toggle.color.a / 255.0f
                    };
                    ImGui::ColorEdit4("Color", col);
                    toggle.color = {int(col[0] * 255), int(col[1] * 255), int(col[2] * 255), int(col[3] * 255)};

                    ImGui::EndDisabled();
                    break;
                }


            }
        }

        ImGui::End();
    }
}

void EliseApp::handle_input() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (audio_manager.isPlaying()) {
            stop_audio();
        } else {
            play_audio();
        }
    }
}

void EliseApp::update() {
    update_dialogs();
    handle_input();
    update_waveform_viewer();
    update_light_manager();
    update_viewport();
}

void EliseApp::update_waveform_viewer() {
    if (audio_manager.isPlaying()) {
        auto pos = audio_manager.getPlayheadPosition();
        waveform_viewer.set_cursor_position(pos);
    }
}

void EliseApp::update_light_manager() {
    if (audio_manager.isPlaying()) {
        auto pos = audio_manager.getPlayheadPosition();
        light_manager.update(pos);
    }
}

void EliseApp::update_viewport() {
    if (audio_manager.isPlaying()) {
        viewport.setColors(light_manager.getLightStates());
    }
}

void EliseApp::play_audio() {

    compile_commands();
    audio_manager.play(size_t(waveform_viewer.get_cursor_position()), playback_speed);

}

void EliseApp::stop_audio() {
    audio_manager.stop();
}

void EliseApp::order_keyframes() {
    std::sort(keyframes.begin(), keyframes.end(), compare);
    build_keyframe_uuid_to_index_map();
}

void EliseApp::build_keyframe_uuid_to_index_map() {
    keyframe_uuid_to_index.clear();

    for (size_t i = 0; i < keyframes.size(); ++i) {
        keyframe_uuid_to_index[keyframes[i].uuid] = i;
    }
}

void EliseApp::key_frame_creation_callback(int64_t sample) {
    max_keyframe_uuid++;
    keyframes.push_back(Keyframe{sample, max_keyframe_uuid});

    // Create empty command
    keyframe_uuid_to_commands[max_keyframe_uuid].push_back(Command{});

    order_keyframes();
    update_keyframes();

    waveform_viewer.set_selected_keyframe(max_keyframe_uuid);
    selected_keyframe_uuid = max_keyframe_uuid;
    selected_command = 0;
}

void EliseApp::key_frame_deletion_callback(int64_t keyframe_uuid) {
    auto index = keyframe_uuid_to_index[keyframe_uuid];

    keyframes.erase(keyframes.begin() + index);
    keyframe_uuid_to_commands.erase(keyframe_uuid);
    build_keyframe_uuid_to_index_map();

    update_keyframes();
    selected_keyframe_uuid = -1;
}


void EliseApp::key_frame_drag_callback(int64_t keyframe_uuid, int64_t new_sample) {
    auto& keyframe = keyframes[keyframe_uuid_to_index[keyframe_uuid]];
    keyframe.trigger_sample = new_sample;
    for (auto & command: keyframe_uuid_to_commands[selected_keyframe_uuid]) {retimeCommand(command, new_sample);}
    order_keyframes();
    update_keyframes();
}

void EliseApp::key_frame_selection_callback(int64_t keyframe_uuid) {
    selected_keyframe_uuid = keyframe_uuid;
}

void EliseApp::update_keyframes() {
    std::vector<Keyframe> waveform_keyframes;
    waveform_keyframes.reserve(keyframes.size());

    for (auto& keyframe : keyframes) {waveform_keyframes.push_back({keyframe.trigger_sample, keyframe.uuid});}

    waveform_viewer.set_keyframes(waveform_keyframes);
}

void EliseApp::new_group(const std::string &name, const std::vector<size_t> &ids) {
    groups.push_back(Group(name, ids));
}

void EliseApp::on_save() {
    save_project_dialog = std::make_unique<pfd::save_file>(
        "Save ELISE project",
        "",
        std::vector<std::string>{"ELISE project", "*.elise"},
        pfd::opt::none
        );
    is_save_project_dialog_active = true;
}

void EliseApp::on_open_project() {
    open_project_dialog = std::make_unique<pfd::open_file>(
        "Open ELISE project",
        "",
        std::vector<std::string>{"ELISE project", "*.elise"},
        pfd::opt::none
        );
    is_open_project_dialog_active = true;
}

void EliseApp::on_export() {
    export_project_dialog = std::make_unique<pfd::save_file>(
        "Export python builder script",
        "",
        std::vector<std::string>{"Python script", "*.py"},
        pfd::opt::none
        );
    is_export_project_dialog_active = true;
}

void EliseApp::on_load_song() {
    load_song_dialog = std::make_unique<pfd::open_file>(
        "Load MP3 file",
        "",
        std::vector<std::string>{"MP3 file", "*.mp3"},
        pfd::opt::none
        );
    is_load_song_dialog_active = true;
}


void EliseApp::save_project(const std::string &path) {

    ProjectData project_data;
    project_data.keyframes = keyframes;
    project_data.groups = groups;
    project_data.light_count = light_count;
    project_data.sample_rate = audio_manager.getSampleRate();
    project_data.keyframe_uuid_to_commands = keyframe_uuid_to_commands;
    project_data.max_uuid = max_keyframe_uuid;

    save(path, project_data);
}

void EliseApp::load_project(const std::string &path) {
    ProjectData p = load(path);
    keyframes = p.keyframes;
    groups = p.groups;
    keyframe_uuid_to_commands = p.keyframe_uuid_to_commands;
    light_count = p.light_count;
    max_keyframe_uuid = p.max_uuid;

    order_keyframes();
    update_keyframes();
}

void EliseApp::export_project(const std::string &path) {
    order_keyframes();

    ProjectData project_data;
    project_data.keyframes = keyframes;
    project_data.groups = groups;
    project_data.light_count = light_count;
    project_data.sample_rate = audio_manager.getSampleRate();
    project_data.keyframe_uuid_to_commands = keyframe_uuid_to_commands;

    auto content = generate_python_script(project_data);

    auto n_path = ensure_extension(path, ".py");
    if(n_path.size() >= 1) save_python_script(n_path, content);
}

void EliseApp::load_song(const std::string &path) {
    audio_manager.loadMP3(path);
    waveform_viewer.set_waveform_data(audio_manager.getOriginalSamples());
    waveform_viewer.set_sample_rate(audio_manager.getSampleRate());
}

void EliseApp::update_dialogs() {
    if (open_project_dialog && open_project_dialog->ready()) {
        auto filename = open_project_dialog->result();
        load_project(filename.at(0));
        open_project_dialog.reset();
        is_open_project_dialog_active = false;
    } else if (not open_project_dialog) is_open_project_dialog_active = false;

    if (load_song_dialog && load_song_dialog->ready()) {
        auto filename = load_song_dialog->result();
        load_song(filename.at(0));
        load_song_dialog.reset();
        is_load_song_dialog_active = false;
    } else if (not load_song_dialog) is_load_song_dialog_active = false;

    if (save_project_dialog && save_project_dialog->ready()) {
        auto filename = save_project_dialog->result();
        filename = ensure_extension(filename, ".elise");
        save_project(filename);
        save_project_dialog.reset();
        is_save_project_dialog_active = false;
    } else if (not save_project_dialog) is_save_project_dialog_active = false;

    if (export_project_dialog && export_project_dialog->ready()) {
        auto filename = export_project_dialog->result();
        filename = ensure_extension(filename, ".py");
        export_project(filename);
        export_project_dialog.reset();
        is_export_project_dialog_active = false;
    } else if (not export_project_dialog) is_export_project_dialog_active = false;

    is_dialog_opened = is_open_project_dialog_active || is_load_song_dialog_active || is_save_project_dialog_active || is_export_project_dialog_active;
}

