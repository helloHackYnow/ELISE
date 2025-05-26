//
// Created by victor on 26/05/25.
//

#include "EliseApp.h"


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
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

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

void EliseApp::draw() {
    draw_menu_bar();
    draw_project_manager();
    draw_player();
    waveform_viewer.draw();
}

void EliseApp::draw_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New");
            ImGui::MenuItem("Open");
            ImGui::Separator();
            ImGui::MenuItem("Save");
            ImGui::MenuItem("Save As");
            ImGui::Separator();
            ImGui::MenuItem("Export");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Project Manager", nullptr, &is_project_manager_visible);
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(window, true);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EliseApp::draw_project_manager() {
    if (ImGui::Begin("Project Manager", &is_project_manager_visible)) {
        ImGui::Text("Project Manager");

        if (ImGui::Button("Load song")) {
            const char* filters[] = { "*.mp3" };
            const char* filename = tinyfd_openFileDialog("Open MP3", "./ressources", 1, filters, "MP3 Files", 0);

            if (filename) {
                audio_manager.loadMP3(filename);
                waveform_viewer.set_waveform_data(audio_manager.getOriginalSamples());
                waveform_viewer.set_sample_rate(audio_manager.getSampleRate());
            }
        }

        ImGui::End();
    }
}

void EliseApp::draw_player() {
    if (ImGui::Begin("Player")) {
        ImGui::Text("Player");
        if (ImGui::Button("Play")) {
            audio_manager.play(size_t(waveform_viewer.get_cursor_position()));
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            audio_manager.pause();
        }

        ImGui::End();
    }
}

void EliseApp::update() {
    update_waveform_viewer();
}

void EliseApp::update_waveform_viewer() {
    if (audio_manager.isPlaying()) {
        auto pos = audio_manager.getPlayheadPosition();
        waveform_viewer.set_cursor_position(pos);
    }
}
