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
    }

    unsigned int Renderer::GetTexture() {
        return fbo_output.GetTexture();
    }

    void Renderer::Render(const std::array<glm::vec4, 12> &windows_colors) {

        glViewport(0, 0, viewport_width, viewport_height);

        fbo_output.Use();
        glClearColor(1.f, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        fbo_internal.Use(); // Bind fbo_internal


        glClearColor(1.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO : draw the windows
        light_shader.use();
        glBindVertexArray(quadVAO);

        for (int i = 0; i < 12; ++i) {
            glm::vec2 pos; float scale = 0.2f;
            if (i < 5) pos = { -0.8f + 0.3f * i,  0.5f };
            else if (i < 7) pos = { -0.2f + 0.3f * (i - 5), 0.0f };
            else pos = { -0.5f + 0.3f * (i - 7), -0.5f };

            light_shader.setVec4("uColor", glm::vec4(10.0f, 10.0f, 10.0f, 1.0f));
            light_shader.setVec2("uPosition", glm::vec2(pos[0], pos[1]));
            light_shader.setFloat("uScale", scale);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        //Copy fbo_internal to postProcess
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_internal.ID());

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_postProcess.ID());

        glBlitFramebuffer(0, 0, viewport_width, viewport_height, 0, 0, viewport_width, viewport_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);


        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // Unbin fbo
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        bloom.Apply(fbo_internal, fbo_output, viewport_width, viewport_height);
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
            -0.5f, -0.5f,
             0.5f, -0.5f,
            -0.5f,  0.5f,
            -0.5f,  0.5f,
             0.5f,  0.5f,
             0.5f, -0.5f,
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

