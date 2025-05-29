//
// Created by victor on 26/05/25.
//

#include "EliseApp.h"

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
    glfwSwapInterval(1); // Enable vsync

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

    new_group("all", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
    new_group("up", {0, 1, 2, 3, 4});
    new_group("mid", {5, 6});
    new_group("down", {7, 8, 9, 10, 11});
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
        for (auto & command: keyframe.commands) {

            retimeCommand(command, keyframe.trigger_sample);

            commands.push_back(command);
        }
    }

    std::reverse(commands.begin(), commands.end());

    light_manager.reset();
    light_manager.setCommandStack(commands);
}

void EliseApp::draw() {
    draw_menu_bar();
    draw_player();
    draw_keyframe_edition_window();
    draw_command_edition_window();
    waveform_viewer.draw();
    viewport.draw();

}

void EliseApp::draw_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");
            if (ImGui::MenuItem("Open")) on_load();
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
        if (ImGui::Button("Load audio")) {
            const char* filters[] = { "*.mp3" };
            const char* filename = tinyfd_openFileDialog("Open MP3", "./ressources", 1, filters, "MP3 Files", 0);

            if (filename) {
                audio_manager.loadMP3(filename);
                waveform_viewer.set_waveform_data(audio_manager.getOriginalSamples());
                waveform_viewer.set_sample_rate(audio_manager.getSampleRate());
            }
        }

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

        if (selected_keyframe >= 0) {
            auto & keyframe = keyframes[selected_keyframe];

            std::vector<std::string> commands;
            std::vector<const char*> listbox_buff;

            for (auto & command: keyframe.commands) {
                commands.push_back("Command on group " + groups[command.group_id].name);
                listbox_buff.push_back(commands.back().c_str());
            }

            ImGui::ListBox("###", &selected_command, listbox_buff.data(), listbox_buff.size(), 6);



            if (ImGui::Button("Add")) {
                keyframe.commands.push_back(Command{});
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(selected_command < 0 || selected_command >= keyframe.commands.size());
            if (ImGui::Button("Delete")) {
                keyframe.commands.erase(keyframes[selected_keyframe].commands.begin() + selected_command);
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
        if (selected_keyframe < 0 || selected_command < 0 || selected_command >= keyframes[selected_keyframe].commands.size()) {
            ImGui::BeginDisabled();
            ImGui::Text("No command selected");
            ImGui::EndDisabled();
        } else {
            auto & command = keyframes[selected_keyframe].commands[selected_command];

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
}

void EliseApp::key_frame_creation_callback(float sample) {

    keyframes.push_back(Keyframe{int(sample), {Command{}}});
    order_keyframes();
    update_keyframes();

    selected_keyframe = -1;
}

// TODO : optimization
void EliseApp::key_frame_deletion_callback(int keyframe_index) {
    keyframes.erase(keyframes.begin() + keyframe_index);
    update_keyframes();
    selected_keyframe = -1;
}

void EliseApp::key_frame_drag_callback(int keyframe_index, int64_t new_sample) {
    keyframes[keyframe_index].trigger_sample = new_sample;
    for (auto & command: keyframes[keyframe_index].commands) {retimeCommand(command, new_sample);}
    //order_keyframes();
    update_keyframes();
}

void EliseApp::key_frame_selection_callback(int keyframe_index) {
    selected_keyframe = keyframe_index;
}

void EliseApp::update_keyframes() {
    std::vector<float> waveform_keyframes;
    waveform_keyframes.reserve(keyframes.size());

    for (auto& keyframe : keyframes) {waveform_keyframes.push_back(float(keyframe.trigger_sample));}

    waveform_viewer.set_keyframes(waveform_keyframes);
}

void EliseApp::new_group(const std::string &name, const std::vector<size_t> &ids) {
    groups.push_back(Group(name, ids));
}

void EliseApp::on_save() {
    const char* filters[] = { "*.elise" };
    const char* filename = tinyfd_saveFileDialog("Save ELISE project", nullptr, 1, filters, "ELISE project file");
    if(filename) save_project(filename);
}

void EliseApp::on_load() {
    const char* filters[] = { "*.elise" };
    const char* filename = tinyfd_openFileDialog("Open ELISE project", nullptr, 1, filters, "ELISE project file", false);
    if(filename) load_project(filename);
}

void EliseApp::on_export() {
    order_keyframes();

    ProjectData project_data;
    project_data.keyframes = keyframes;
    project_data.groups = groups;
    project_data.light_count = light_count;
    project_data.sample_rate = audio_manager.getSampleRate();

    std::cout << generate_python_script(project_data) << std::endl;
}

void EliseApp::save_project(const std::string &path) {
    ProjectData project_data;
    project_data.keyframes = keyframes;
    project_data.groups = {};

    save(path, project_data);
}

void EliseApp::load_project(const std::string &path) {
    ProjectData p = load(path);
    keyframes = p.keyframes;

    order_keyframes();
    update_keyframes();
}

