//
// Created by victor on 26/05/25.
//

#include "EliseApp.h"

#include <filesystem>
#include <algorithm> // Ensure this is included at the top of the file

#include "imgui_internal.h"
#include "../fonts/icon_font.h"

#undef min
#undef max



EliseApp::EliseApp() {
}

bool EliseApp::init() {

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // GL 3.3 + GLSL 330
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))  // tie window context to glad's opengl funcs
        throw("Unable to context to OpenGL");

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
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup ImGui style
    ImFontConfig font_config;
    font_config.OversampleH = 3;
    font_config.OversampleV = 1;
    font_config.PixelSnapH = false;
    font_config.FontDataOwnedByAtlas = false;
    font_config.MergeMode = false;

    io.Fonts->AddFontFromMemoryTTF(SourceCodePro_Semibold_ttf, SourceCodePro_Semibold_ttf_len, 18.0f, &font_config);

    font_config.MergeMode = true;
    static const ImWchar icon_range[] = { 0xf000, 0xf8ff, 0 };

    io.Fonts->AddFontFromMemoryCompressedTTF(icon_font_compressed_data, icon_font_compressed_size, 18.f, &font_config,icon_range);

    // Setup Dear ImGui style
    setBessDarkColors();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    waveform_viewer.keyframe_creation_callback      = [this](float arg){keyframe_creation_callback(arg);};
    waveform_viewer.keyframe_deletion_callback      = [this](){keyframe_deletion_callback();};
    waveform_viewer.keyframe_drag_callback          = [this](int64_t arg2){keyframe_drag_callback(arg2);};
    waveform_viewer.keyframe_selection_callback     = [this](int arg){keyframe_selection_callback(arg);};
    waveform_viewer.reset_selection_callback        = [this](){reset_selection_callback();};
    waveform_viewer.keyframe_unselection_callback   = [this](int arg){keyframe_unselection_callback(arg);};

    init_groups();
    init_light_manager();

    renderer.InitRenderer(1000, 500);

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

		configure_dockspace();

        draw();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        glfwSwapBuffers(window);

        if (is_exporting) {
            for (int i = 0; i < 5; ++i) {
                update_export();
            }
        }

    }
}

void EliseApp::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void EliseApp::configure_dockspace()
{
    static bool first_time = true;
	auto dock_id = ImGui::DockSpaceOverViewport();

    if (first_time)
    {
        first_time = false;

        ImGui::DockBuilderRemoveNode(dock_id); // Clear out existing layout
        ImGui::DockBuilderAddNode(dock_id); // Add empty node
        ImGui::DockBuilderSetNodeSize(dock_id, ImGui::GetMainViewport()->Size);

        auto dock_id_right = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Right, 0.2f, nullptr, &dock_id);
        auto dock_id_bottom = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Down, 0.4f, nullptr, &dock_id);
		auto dock_id_left = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Left, 0.2f, nullptr, &dock_id);

		auto dock_id_right_up = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 0.5f, nullptr, &dock_id_right);

        

        ImGui::DockBuilderDockWindow("Audio player", dock_id_left);
        ImGui::DockBuilderDockWindow("Command", dock_id_right);
		ImGui::DockBuilderDockWindow("Keyframe", dock_id_right_up);
        ImGui::DockBuilderDockWindow("Waveform Viewer", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Preview", dock_id);
        ImGui::DockBuilderFinish(dock_id);
    }

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

            if (keyframe.is_enabled) commands.push_back(command);
        }
    }

    std::reverse(commands.begin(), commands.end());

    light_manager.reset();
    light_manager.setCommandStack(commands);
}

void EliseApp::draw() {

    ImGui::BeginDisabled(is_dialog_opened | is_exporting);

    if (!is_exporting) {
        draw_menu_bar();
        draw_player();
        draw_keyframe_edition_window();
        draw_command_edition_window();
        waveform_viewer.draw();
        draw_viewport();
    }


    // Notifications style setup
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f); // Disable round borders
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f); // Disable borders

    // Notifications color setup
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f)); // Background color


    // Main rendering function
    ImGui::RenderNotifications();


    //——————————————————————————————— WARNING ———————————————————————————————
    // Argument MUST match the amount of ImGui::PushStyleVar() calls
    ImGui::PopStyleVar(2);
    // Argument MUST match the amount of ImGui::PushStyleColor() calls
    ImGui::PopStyleColor(1);

    ImGui::EndDisabled();

    if (is_exporting) {
        draw_export_pop_up();
    }
}

void EliseApp::draw_export_pop_up() {
    // Get the main viewport size and position
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Desired window size (optional, define your own size)
    ImVec2 window_size = ImVec2(200, 100); // Adjust size as needed
    ImVec2 window_pos = ImVec2(
        viewport->Pos.x + (viewport->Size.x - window_size.x) * 0.5f,
        viewport->Pos.y + (viewport->Size.y - window_size.y) * 0.5f
    );

    // Set the position and size *before* Begin
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

    auto popup_flag = ImGuiWindowFlags_NoDocking
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove;

    ImGui::Begin("Export popup", nullptr, popup_flag);
    ImGui::Text("Exporting :");
    float progress = float(current_frame) / float(max_frame);
    ImGui::ProgressBar(progress);
    ImGui::End();

    ImGui::PopStyleColor();
}

void EliseApp::draw_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) on_open_project();
            ImGui::Separator();
            ImGui::BeginDisabled(!is_loaded_from_file);
            if (ImGui::MenuItem("Save", "Ctrl-S")) on_save();
            ImGui::EndDisabled();
            if (ImGui::MenuItem("Save as")) on_save_as();
            ImGui::Separator();
            if (ImGui::MenuItem("Export script")) on_export();
            if (ImGui::MenuItem("Export video")) on_export_video();

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

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Sample rate: %i /s", sample_rate);

    }

    ImGui::End();
}

void EliseApp::draw_viewport() {
    ImGui::Begin("Preview");
    static int m_width, m_height = 1;
    int width = ImGui::GetContentRegionAvail().x;
    int height = ImGui::GetContentRegionAvail().y;

    if (m_width != width || m_height != height) {
        m_width = width;
        m_height = height;
    }
    renderer.setViewport(m_width, m_height);

    auto& colors = light_manager.getLightStates();
    std::array<glm::vec4, 12> arr_colors;
    for (int i = 0; i < 12; ++i) {
        auto& color = colors[i];
        arr_colors.at(i) = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
    }

    if (!is_exporting) renderer.Render(arr_colors);

    ImGui::Image(renderer.GetTexture(), ImVec2((float)m_width, (float)m_height), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}

void EliseApp::draw_keyframe_edition_window() {
    if (ImGui::Begin("Keyframe", &is_keyframe_edition_window_visible)) {

		ImGui::Spacing();
        ImGui::Text("Keyframe");

        ImGui::Separator();

        if (selected_keyframes.empty()) {
            ImGui::BeginDisabled();
            ImGui::Text("No keyframe selected");
            ImGui::EndDisabled();
        } else if (selected_keyframes.size() > 1) {
            ImGui::BeginDisabled();
            ImGui::Text("Multiple keyframes selected");
            ImGui::EndDisabled();

            int are_locked = -1; // -1: start value, 0: all locked, 1: all unlocked, 2: mixed
            int are_enabled = -1; // -1: start value, 0: all enabled, 1: all disabled, 2: mixed
            for (int64_t selected_keyframe: selected_keyframes) {
                auto& keyframe = keyframes[keyframe_uuid_to_index[selected_keyframe]];

                if (keyframe.is_locked) {
                    if (are_locked == -1 || are_locked == 0) are_locked = 0;
                    else are_locked = 2;
                } else {
                    if (are_locked == -1 || are_locked == 1) are_locked = 1;
                    else are_locked = 2;
                }

                if (keyframe.is_enabled) {
                    if (are_enabled == -1 || are_enabled == 0) are_enabled = 0;
                    else are_enabled = 2;
                } else {
                    if (are_enabled == -1 || are_enabled == 1) are_enabled = 1;
                    else are_enabled = 2;
                }
            }

            if (are_locked != 1)
            {
                if (ImGui::Button((const char*)u8"\uf023"))
                {
                    for (int64_t selected_keyframe: selected_keyframes) {
                        auto& keyframe = keyframes[keyframe_uuid_to_index[selected_keyframe]];
                        keyframe.is_locked = false;
                    }
                    update_keyframes();
                }
            }
            else
            {
                if (ImGui::Button((const char*)u8"\uf3c1"))
                {
                    for (int64_t selected_keyframe: selected_keyframes) {
                        auto& keyframe = keyframes[keyframe_uuid_to_index[selected_keyframe]];
                        keyframe.is_locked = true;
                    }
                    update_keyframes();
                }
            }

            ImGui::SameLine();

            bool enabled = are_enabled == 0;
            if (ImGui::Checkbox("Enabled", &enabled)) {
                for (int64_t selected_keyframe: selected_keyframes) {
                    auto& keyframe = keyframes[keyframe_uuid_to_index[selected_keyframe]];
                    keyframe.is_enabled = enabled;
                }
                update_keyframes();
            }


        } else {
            std::vector<std::string> commands_str;
            std::vector<const char*> listbox_buff;

            auto selected_keyframe_uuid = *selected_keyframes.begin();
            auto& keyframe = keyframes.at(keyframe_uuid_to_index.at(selected_keyframe_uuid));

            auto& commands = keyframe_uuid_to_commands[selected_keyframe_uuid];

            // Draw a lock / unlock button
            if (keyframe.is_locked)
            {
                if (ImGui::Button((const char*)u8"\uf023"))
                {
                    keyframe.is_locked = false;
                    update_keyframes();
                }
            }
            else
            {
                if (ImGui::Button((const char*)u8"\uf3c1"))
                {
                    keyframe.is_locked = true;
                    update_keyframes();
                }
            }

            ImGui::SameLine();
            if (ImGui::Checkbox("Enabled", &keyframe.is_enabled)) update_keyframes();

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

        }

    }
    ImGui::End();
}

void EliseApp::draw_command_edition_window() {
    if (ImGui::Begin("Command", &is_command_edition_window_visible)) {
        if (selected_keyframes.empty()) {
            ImGui::BeginDisabled();
            ImGui::Text("No command selected");
            ImGui::EndDisabled();
        } else if (selected_keyframes.size() > 1) {
            ImGui::BeginDisabled();
            ImGui::Text("Multiple keyframes selected");
            ImGui::EndDisabled();
        } else {
            ImGui::Spacing();
            auto selected_keyframe_uuid = *selected_keyframes.begin();
            auto& command = keyframe_uuid_to_commands[selected_keyframe_uuid][selected_command];
            auto& keyframe = keyframes.at(keyframe_uuid_to_index[selected_keyframe_uuid]);

            ImGui::Text("Command %d", selected_command);

            ImGui::SameLine();
            float copy_btn_width = ImGui::CalcTextSize((const char *)u8"\uf24d").x + ImGui::GetStyle().FramePadding.x * 2.f;
            float paste_btn_width = ImGui::CalcTextSize((const char *)u8"\uf0ea").x + ImGui::GetStyle().FramePadding.x * 2.f;
            float widthNeeded = copy_btn_width + ImGui::GetStyle().ItemSpacing.x + paste_btn_width;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - widthNeeded);

            if (ImGui::Button((const char*)u8"\uf24d")) copy_command(command);
            ImGui::SameLine();

            ImGui::BeginDisabled(!has_copied_command);
            if (ImGui::Button((const char*)u8"\uf0ea")) command = copied_command;
            ImGui::EndDisabled();

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

                    int duration_in_ms = (gradient.duration * 1000) / sample_rate;
                    ImGui::DragInt("Duration ms", &duration_in_ms);
                    if (duration_in_ms < 0) duration_in_ms = 0;
                    gradient.duration = (duration_in_ms * sample_rate) / 1000;

                    waveform_viewer.set_gradient_preview(keyframe.trigger_sample, gradient.duration);

                    ImGui::Spacing();

                    ImGui::Text("Color gradient");
                    ImGui::Separator();

                    color_picker("Start color", gradient.start_color);

                    color_picker("End color", gradient.end_color);
                    break;
                }

                case AnimationKind::toggle : {

                    auto & toggle = command.animation.toggle;

                    ImGui::Checkbox("Toggle on", &toggle.is_on);
                    ImGui::Spacing();

                    ImGui::BeginDisabled(!toggle.is_on);

                    ImGui::Text("Color");
                    ImGui::Separator();

                    color_picker("Color", toggle.color);

                    ImGui::EndDisabled();
                    break;
                }

                case AnimationKind::blink : {
                    auto & blink = command.animation.blink;

                    int period_in_ms = (blink.period) * 1000 / sample_rate;
                    ImGui::DragInt("Period ms", &period_in_ms);
                    if (period_in_ms < 0) period_in_ms = 0;
                    blink.period = period_in_ms * sample_rate / 1000;

                    color_picker("On color", blink.on_color);

                    color_picker("Off color", blink.off_color);
                    break;
                }
            }
        }
    }

    ImGui::End();
}

void EliseApp::handle_input() {
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        if (audio_manager.isPlaying()) {
            stop_audio();
        } else {
            play_audio();
        }
    }

    if (ImGui::IsKeyDown(ImGuiKey_ModCtrl) && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (is_loaded_from_file) on_save();
        else on_save_as();
    }
}

void EliseApp::update() {
    if (is_exporting) return;
    update_dialogs();
    handle_input();
    update_waveform_viewer();
    update_light_manager();

    waveform_viewer.set_selected_keyframe(selected_keyframes);
}

void EliseApp::update_waveform_viewer() {
    if (audio_manager.isPlaying()) {
        auto pos = audio_manager.getPlayheadPosition();
        waveform_viewer.set_cursor_position(pos);
    }
}

void EliseApp::update_light_manager() {
    if (audio_manager.isPlaying() && !is_exporting) {
        auto pos = audio_manager.getPlayheadPosition();
        light_manager.update(pos);
    }
}

void EliseApp::play_audio() {
    compile_commands();
    audio_manager.play(int64_t(waveform_viewer.get_cursor_position()), playback_speed);
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

void EliseApp::keyframe_creation_callback(int64_t sample) {
    max_keyframe_uuid++;
    keyframes.push_back(Keyframe{sample, max_keyframe_uuid});

    // Create empty command
    keyframe_uuid_to_commands[max_keyframe_uuid].push_back(Command{});

    order_keyframes();
    update_keyframes();

    waveform_viewer.set_selected_keyframe(selected_keyframes);
    selected_keyframes.clear();
    selected_keyframes.insert(selected_keyframes.begin(), max_keyframe_uuid);
    selected_command = 0;
}

void EliseApp::keyframe_deletion_callback() {

    // Build the list of index to delete
    std::vector<int> to_delete;
    to_delete.reserve(selected_keyframes.size());
    for (int64_t selected_keyframe: selected_keyframes) {
        to_delete.push_back(keyframe_uuid_to_index[selected_keyframe]);
        keyframe_uuid_to_index.erase(selected_keyframe);
    }
    std::sort(to_delete.begin(), to_delete.end());
    std::reverse(to_delete.begin(), to_delete.end());

    for (int keyframe_index: to_delete) {
        keyframes.erase(keyframes.begin() + keyframe_index);
    }

    build_keyframe_uuid_to_index_map();
    update_keyframes();
    selected_keyframes.clear();
}

void EliseApp::keyframe_drag_callback(int64_t delta_sample) {
    // Check if one of the selected keyframe is locked
    for (int64_t selected_keyframe: selected_keyframes) {
        if (keyframes.at(keyframe_uuid_to_index[selected_keyframe]).is_locked) return;
    }

    for (int64_t selected_keyframe: selected_keyframes) {
        auto& keyframe = keyframes.at(keyframe_uuid_to_index[selected_keyframe]);
        keyframe.trigger_sample += delta_sample;

        for (auto& command : keyframe_uuid_to_commands[selected_keyframe]) { retimeCommand(command, keyframe.trigger_sample); }
    }

    order_keyframes();
    update_keyframes();
}

void EliseApp::keyframe_selection_callback(int64_t keyframe_uuid) {
    selected_keyframes.insert(keyframe_uuid);
}

void EliseApp::reset_selection_callback() {
    selected_keyframes.clear();
}

void EliseApp::keyframe_unselection_callback(int64_t keyframe_uuid) {
    selected_keyframes.erase(keyframe_uuid);
}

void EliseApp::update_keyframes() {
    std::vector<Keyframe> waveform_keyframes;
    waveform_keyframes.reserve(keyframes.size());

    for (auto& keyframe : keyframes) {waveform_keyframes.push_back({keyframe.trigger_sample, keyframe.uuid, keyframe.is_locked, keyframe.is_enabled});}

    waveform_viewer.set_keyframes(waveform_keyframes);
}

void EliseApp::new_group(const std::string &name, const std::vector<size_t> &ids) {
    groups.push_back(Group(name, ids));
}

void EliseApp::on_save() {
    if (is_loaded_from_file) save_project(filepath);
}

void EliseApp::on_save_as() {
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

void EliseApp::on_export_video() {
    export_video_dialog = std::make_unique<pfd::save_file>(
        "Export project to video",
        "",
        std::vector<std::string>{"MP4 file", "*.mp4"},
        pfd::opt::none);
    is_export_video_dialog_active = true;
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
    is_loaded_from_file = true;
    filepath = path;

    ImGui::InsertNotification({ImGuiToastType::Info, 3000, "The project was saved !"});
}

void EliseApp::load_project(const std::string &path) {
    ProjectData p;
    bool error = false;

    try {
        p = load(path);
    } catch (const std::exception &e) {
        ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Not a valid ELISE file !"});
        error = true;
    }

    if (!error) {
        keyframes = p.keyframes;
        groups = p.groups;
        keyframe_uuid_to_commands = p.keyframe_uuid_to_commands;
        light_count = p.light_count;
        max_keyframe_uuid = p.max_uuid;

        order_keyframes();
        update_keyframes();
        is_loaded_from_file = true;
        filepath = path;

        ImGui::InsertNotification({ImGuiToastType::Info, 3000, "The project was loaded !"});
    }




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
    auto& data = audio_manager.getOriginalSamples();
    waveform_viewer.set_waveform_data(data);
    waveform_viewer.set_sample_rate(audio_manager.getSampleRate());
    sample_rate = audio_manager.getSampleRate();
    sample_count = data.size();
}

void EliseApp::update_dialogs() {
    if (open_project_dialog && open_project_dialog->ready()) {
        auto filename = open_project_dialog->result();
        if(filename.size() > 0 ){
            load_project(filename.at(0));
        }
        open_project_dialog.reset();
        is_open_project_dialog_active = false;
    } else if (not open_project_dialog) is_open_project_dialog_active = false;

    if (load_song_dialog && load_song_dialog->ready()) {
        auto filename = load_song_dialog->result();
        if(filename.size() > 0){
            load_song(filename.at(0));
        }
        load_song_dialog.reset();
        is_load_song_dialog_active = false;
    } else if (not load_song_dialog) is_load_song_dialog_active = false;

    if (save_project_dialog && save_project_dialog->ready()) {
        auto filename = save_project_dialog->result();
        if(filename.length() > 0){
            filename = ensure_extension(filename, ".elise");
            save_project(filename);
        }
        save_project_dialog.reset();
        is_save_project_dialog_active = false;
    } else if (not save_project_dialog) is_save_project_dialog_active = false;

    if (export_project_dialog && export_project_dialog->ready()) {
        auto filename = export_project_dialog->result();
        if(filename.length() > 0){
            filename = ensure_extension(filename, ".py");
            export_project(filename);
        }
        export_project_dialog.reset();
        is_export_project_dialog_active = false;
    } else if (not export_project_dialog) is_export_project_dialog_active = false;

    if (export_video_dialog && export_video_dialog->ready()) {
        auto filename = export_video_dialog->result();
        if(filename.length() > 0) {
            filename = ensure_extension(filename, ".mp4");
            start_export(filename);
        }
        export_video_dialog.reset();
        is_export_video_dialog_active = false;
    }

    is_dialog_opened = is_open_project_dialog_active
                        || is_load_song_dialog_active
                        || is_save_project_dialog_active
                        || is_export_project_dialog_active
                        || is_export_video_dialog_active;
}

void EliseApp::copy_color(const Color &color) {
    has_copied_color = true;
    copied_color = color;
}

void EliseApp::copy_command(const Command &command) {
    has_copied_command = true;
    copied_command = command;
}

void EliseApp::copy_commands(const std::vector<Command> &commands) {
    has_copied_commands = true;
    copied_commands = commands;
}

void EliseApp::color_picker(const char *label, Color &color) {



    ImGui::PushID(label);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    if (ImGui::Button((const char *)u8"\uf24d")) copy_color(color); // Copy button
    ImGui::SameLine();
    ImGui::BeginDisabled(!has_copied_color);
    if (has_copied_color) ImGui::PushStyleColor(ImGuiCol_Text, get_vec(copied_color));
    if (ImGui::Button((const char *)u8"\uf0ea")) color = copied_color;
    if (has_copied_color) ImGui::PopStyleColor(1);
    ImGui::EndDisabled();
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushItemWidth(40);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0)),
    ImGui::PushID("r");
    ImGui::InputInt("", &color.r, 0);
    ImGui::PopID();
    ImGui::SameLine();
    color.r = std::min(std::max(color.r, 0), 255);

    ImGui::PushID("g");
    ImGui::InputInt("", &color.g, 0);
    ImGui::PopID();
    ImGui::SameLine();
    color.g = std::min(std::max(color.g, 0), 255);

    ImGui::PushID("b");
    ImGui::InputInt("", &color.b, 0);
    ImGui::PopID();
    color.b = std::min(std::max(color.b, 0), 255);
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    ImGui::SameLine();
    float col[] = {
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f
    };

    ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_NoInputs);
    color = {int(col[0] * 255), int(col[1] * 255), int(col[2] * 255), int(col[3] * 255)};

    ImGui::PopID();
}

void EliseApp::start_export(const std::string &path) {
    stop_audio();

    is_exporting = true;

    order_keyframes();

    auto last_k = keyframes.back();

    compile_commands();

    encoder = new MP4Encoder(path, 750, 370, export_framerate, sample_rate);
    encoder->addAudio(audio_manager.getOriginalSamples());
    current_frame = 0;

    max_frame = (last_k.trigger_sample * export_framerate / sample_rate);
}

void EliseApp::export_frame() {
    if (!is_exporting) return;

    int current_sample = current_frame * sample_rate / export_framerate;

    renderer.setViewport(750, 370);
    light_manager.updateAnimations(current_sample);
    light_manager.updateLightStates(current_sample);

    auto lights = light_manager.getLightStates();

    auto vec_light = std::array<glm::vec4, 12>{};
    int i = 0;
    for (auto light: lights) {
        vec_light.at(i) = {light.r / 255.f, light.g / 255.f, light.b / 255.f, light.a / 255.f};
        i++;
    }

    renderer.Render(vec_light);
    encoder->addOpenGLFrame(renderer.GetFboOut());
}

void EliseApp::update_export() {
    if (!is_exporting) return;
    if (current_frame < max_frame) {
        export_frame();
        current_frame++;
    }
    else {
        is_exporting = false;
        encoder->finalize();
        delete encoder;

        ImGui::InsertNotification({ImGuiToastType::Success, 5000, "Video exported !"});
    }
}

