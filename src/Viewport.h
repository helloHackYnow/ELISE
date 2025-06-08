//
// Created by victor on 26/05/25.
//

#ifndef VIEWPORT_H
#define VIEWPORT_H


#include <array>
#include <string>
#include <../libs/glad/include/glad/glad.h>
#include <../libs/glm/glm.hpp>
#include <imgui.h>

#include "LightManager.h"
#include "2D renderer/Renderer.h"


// Viewport: renders 12 lights into an offscreen OpenGL texture with optional bloom
// and displays the result in an ImGui window named "preview".
class Viewport {
public:
    // Constructor: initialize framebuffer, shaders, and default parameters.
    // width/height define initial texture size; shaderDir is the path prefix for shader files.
    Viewport();

    void init(int width, int height);

    // Set RGBA colors for the 12 lights (in row order: 5,2,5).
    void setColors(const std::vector<Color>& colors);
    void setColors(const std::array<glm::vec4, 12>& colors);

    void draw();

    // Resize the offscreen texture (call when ImGui preview window size changes).
    void resize(int width, int height);
private:
    std::array<glm::vec4, 12> m_colors;
    Odin::Renderer renderer;

    int m_width;
    int m_height;

};


#endif //VIEWPORT_H
