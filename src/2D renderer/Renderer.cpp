//
// Created by victor on 05/06/25.
//

#include "Renderer.h"

namespace Odin {
    Renderer::Renderer() = default;

    void Renderer::InitRenderer(int width, int height) {
        light_shader = GShader(light_vert, light_frag, true);

        InitVAO();

        this->setViewport(width, height);

        fbo_internal.Init(width, height, true, 1);
        fbo_postProcess.Init(width, height, true, 1);
        fbo_output.Init(width, height, false, 1);

        bloom.Init();
        dummy_effect.Init();
    }

    unsigned int Renderer::GetTexture() {
        return fbo_output.GetTexture();
    }

    const FrameBuffer& Renderer::GetFboOut() {
        return fbo_output;
    }

    void Renderer::Render(const std::array<glm::vec4, 12> &windows_colors) {

        glViewport(0, 0, viewport_width, viewport_height);

        // First, render to the internal framebuffer
        fbo_internal.Use();
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw the lights
        light_shader.use();
        glBindVertexArray(quadVAO);

        const float y_top = 0.6f;
        const float y_mid = 0.0f;
        const float y_bot = -0.6f;

        const float x_spacing = 0.3f;

        for (int i = 0; i < 12; ++i) {
            glm::vec2 pos; float scale = 0.24f;
            if (i < 5) pos = { -0.8f + x_spacing * i,  y_top };
            else if (i < 7) pos = { -0.2f + x_spacing * (i - 5), y_mid };
            else pos = { -0.5f + x_spacing * (i - 7), y_bot };

            light_shader.setVec4("uColor", windows_colors.at(i) * 5.f);
            light_shader.setVec2("uPosition", glm::vec2(pos[0], pos[1]));
            light_shader.setFloat("uScale", scale);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Prepare the output framebuffer
        fbo_output.Use();
        glClearColor(1.f, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Now blit from internal to output
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_internal.ID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_postProcess.ID());
        glBlitFramebuffer(0, 0, viewport_width, viewport_height,
                         0, 0, viewport_width, viewport_height,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // CRITICAL: Restore the default framebuffer binding for ImGui
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // Alternatively, you could apply post-processing effects here
        bloom.Apply(fbo_postProcess, fbo_output, viewport_width, viewport_height);
    }

    void Renderer::setViewport(int width, int height) {
        viewport_height = height;
        viewport_width = width;

        fbo_internal.Rescale(width, height);
        fbo_postProcess.Rescale(width, height);
        fbo_output.Rescale(width, height);
    }

    void Renderer::InitVAO() {
        float quadVertices[] = {
            // pos
            -0.25f, -0.75f,
             0.25f, -0.75f,
            -0.25f,  0.75f,
            -0.25f,  0.75f,
             0.25f,  0.75f,
             0.25f, -0.75f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
}