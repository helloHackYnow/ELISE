//
// Created by victor on 26/05/25.
//

#include "Viewport.h"

#include <iostream>

#include "GLFW/glfw3.h"
Viewport::Viewport(){

}


void Viewport::init(int width, int height) {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))  // tie window context to glad's opengl funcs
        throw("Unable to context to OpenGL");

    renderer.InitRenderer(width, height);
}

void Viewport::setColors(const std::vector<Color> &colors) {
    std::array<glm::vec4, 12> c;
    for (int i = 0; i < 12; ++i) {
        c[i] = glm::vec4(colors[i].r / 255.f, colors[i].g / 255.f, colors[i].b / 255.f, colors[i].a / 255.f);
    }
    setColors(c);
}

void Viewport::setColors(const std::array<glm::vec4, 12> &colors) {
    m_colors = colors;
}

void Viewport::draw() {
    // 5. ImGui preview window
    ImGui::Begin("Preview");
    resize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);

    renderer.Render(m_colors);

    ImGui::Image(renderer.GetTexture(), ImVec2((float)m_width, (float)m_height), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}

void Viewport::resize(int width, int height) {
    if (width == m_width && height == m_height) return;
    m_width = width; m_height = height;

    renderer.setViewport(m_width, m_height);
}
