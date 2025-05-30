//
// Created by victor on 26/05/25.
//

#include "Viewport.h"
#include "shaders.h"

#include <iostream>

#include "GLFW/glfw3.h"

GLuint compileShader(GLenum shaderType, const char *shaderSrc) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, nullptr, buf);
        std::cerr << "Shader compile error: " << buf << std::endl;
    }
    return shader;
}

GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char buf[512];
        glGetProgramInfoLog(prog, 512, nullptr, buf);
        std::cerr << "Program link error: " << buf << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

Viewport::Viewport(){
}

Viewport::~Viewport() {
    glDeleteFramebuffers(1, &sceneFBO_);
    glDeleteTextures(1, &sceneTex_);
    glDeleteVertexArrays(1, &quadVAO_);
    glDeleteBuffers(1, &quadVBO_);
    glDeleteProgram(lightProg_);
    glDeleteProgram(brightProg_);
    glDeleteProgram(blurProg_);
    glDeleteProgram(combineProg_);
    glDeleteFramebuffers(1, &brightFBO_);
    glDeleteTextures(1, &brightTex_);
    glDeleteFramebuffers(2, pingpongFBO_);
    glDeleteTextures(2, pingpongTex_);
}

void Viewport::init(int width, int height) {

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))  // tie window context to glad's opengl funcs
        throw("Unable to context to OpenGL");

    width_ = width;
    height_ = height;

    // default white colors
    for(auto& c : colors_) c = glm::vec4(.0f, 0.f, .0f, 100.0f);

    initShaders();
    initQuad();
    initFramebuffer(width, height);
}

void Viewport::setColors(const std::vector<Color> &colors) {
    std::array<glm::vec4, 12> c;
    for (int i = 0; i < 12; ++i) {
        c[i] = glm::vec4(colors[i].r / 255.f, colors[i].g / 255.f, colors[i].b / 255.f, colors[i].a / 255.f);
    }
    setColors(c);
}

void Viewport::setColors(const std::array<glm::vec4, 12> &colors) {
    colors_ = colors;
}

void Viewport::draw() {
    // 5. ImGui preview window
    ImGui::Begin("Preview");
    resize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
    render();
    ImGui::Image((ImTextureID)(uintptr_t)sceneTex_, ImVec2((float)width_, (float)height_), ImVec2(0, 1), ImVec2(1, 0));
    ImGui::End();
}

void Viewport::render() {
    // 1. Render lights to scene texture
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);
    glViewport(0, 0, width_, height_);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(lightProg_);
    glBindVertexArray(quadVAO_);

    for (int i = 0; i < 12; ++i) {
        glm::vec2 pos; float scale = 0.1f;
        if (i < 5) pos = { -0.8f + 0.3f * i,  0.5f };
        else if (i < 7) pos = { -0.2f + 0.3f * (i - 5), 0.0f };
        else pos = { -0.5f + 0.3f * (i - 7), -0.5f };

        GLint colorLoc = glGetUniformLocation(lightProg_, "uColor");
        GLint posLoc = glGetUniformLocation(lightProg_, "uPosition");
        GLint scaleLoc = glGetUniformLocation(lightProg_, "uScale");
        glUniform4fv(colorLoc, 1, &colors_[i][0]);
        glUniform2fv(posLoc, 1, &pos[0]);
        glUniform1f(scaleLoc, scale);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // 2. Extract bright areas
    glBindFramebuffer(GL_FRAMEBUFFER, brightFBO_);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(brightProg_);
    glBindVertexArray(quadVAO_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex_);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 3. Gaussian blur (ping-pong)
    bool horizontal = true, first = true;
    glUseProgram(blurProg_);
    for (int i = 0; i < blurPasses_; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO_[horizontal]);
        glUniform1i(glGetUniformLocation(blurProg_, "horizontal"), horizontal);

        glBindTexture(GL_TEXTURE_2D, first ? brightTex_ : pingpongTex_[!horizontal]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal = !horizontal;
        first = false;
    }

    // 4. Final composition
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(combineProg_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTex_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongTex_[!horizontal]);
    glUniform1f(glGetUniformLocation(combineProg_, "bloomIntensity"), bloomIntensity_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Viewport::resize(int width, int height) {
    if (width == width_ && height == height_) return;
    width_ = width; height_ = height;
    // delete old and re-init
    glDeleteFramebuffers(1, &sceneFBO_);
    glDeleteTextures(1, &sceneTex_);
    initFramebuffer(width_, height_);
}

void Viewport::initFramebuffer(int width, int height) {
    glGenFramebuffers(1, &sceneFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO_);

    glGenTextures(1, &sceneTex_);
    glBindTexture(GL_TEXTURE_2D, sceneTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex_, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Scene FBO is not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Viewport::initQuad() {
    float quadVertices[] = {
        // pos
        -0.5f, -0.5f,
         0.5f, -0.5f,
        -0.5f,  0.5f,
        -0.5f,  0.5f,
         0.5f,  0.5f,
         0.5f, -0.5f,
    };
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Viewport::initShaders() {
    lightProg_   = linkProgram(compileShader(GL_VERTEX_SHADER, light_vert), compileShader(GL_FRAGMENT_SHADER, light_frag));
    brightProg_  = linkProgram(compileShader(GL_VERTEX_SHADER, bright_vert), compileShader(GL_FRAGMENT_SHADER, bright_frag));
    blurProg_    = linkProgram(compileShader(GL_VERTEX_SHADER, blur_vert), compileShader(GL_FRAGMENT_SHADER, blur_frag));
    combineProg_ = linkProgram(compileShader(GL_VERTEX_SHADER, combine_vert), compileShader(GL_FRAGMENT_SHADER, combine_frag));
}
