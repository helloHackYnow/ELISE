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

GLuint compileShader(GLenum shaderType, const char* shaderSrc);
GLuint linkProgram(GLuint vs, GLuint fs);

// Viewport: renders 12 lights into an offscreen OpenGL texture with optional bloom
// and displays the result in an ImGui window named "preview".
class Viewport {
public:
    // Constructor: initialize framebuffer, shaders, and default parameters.
    // width/height define initial texture size; shaderDir is the path prefix for shader files.
    Viewport();

    // Destructor: cleans up OpenGL resources.
    ~Viewport();

    void init(int width, int height);

    // Set RGBA colors for the 12 lights (in row order: 5,2,5).
    void setColors(const std::vector<Color>& colors);
    void setColors(const std::array<glm::vec4, 12>& colors);

    // Render lights (with bloom) to texture and display in ImGui "preview" window.
    void draw();
    void render();

    // Resize the offscreen texture (call when ImGui preview window size changes).
    void resize(int width, int height);



private:
    void initFramebuffer(int width, int height);
    void initQuad();
    void initShaders();

    // Offscreen rendering
    GLuint sceneFBO_ = 0;
    GLuint sceneTex_ = 0;

    // Fullscreen quad for lighting
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;

    // Shader program IDs
    GLuint lightProg_ = 0;
    GLuint brightProg_ = 0;
    GLuint blurProg_ = 0;
    GLuint combineProg_ = 0;

    int width_, height_;

    // Light colors
    std::array<glm::vec4, 12> colors_;

    // Bloom parameters & resources
    float bloomThreshold_ = 1.0f;
    float bloomIntensity_ = 1.0f;
    int blurPasses_ = 10;
    GLuint brightFBO_ = 0;
    GLuint brightTex_ = 0;
    GLuint pingpongFBO_[2] = {0, 0};
    GLuint pingpongTex_[2] = {0, 0};
};


#endif //VIEWPORT_H
