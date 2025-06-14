//
// Created by victor on 05/06/25.
//

#ifndef RENDERER_H
#define RENDERER_H


#include "Framebuffer.h"
#include "Texture.h"
#include "../LightManager.h"
#include "PostProcessing/Effects/Bloom/Bloom.h"
#include "default_shaders.h"
#include "../../libs/glm/glm.hpp"
#include <array>


namespace Odin {
    class Renderer {
    public:
        Renderer();

        void InitRenderer(int width = 800, int height = 600);

        unsigned int GetTexture();
        const FrameBuffer& GetFboOut();
        void Render(const std::array<glm::vec4, 12>& windows_colors);

        void setViewport(int width, int height);

    private:
        void InitVAO();

        GLuint quadVAO = 0;
        GLuint quadVBO = 0;

        Bloom bloom;
        Effect dummy_effect;

        GShader light_shader;

        FrameBuffer fbo_internal;
        FrameBuffer fbo_postProcess;
        FrameBuffer fbo_output;

        int viewport_width = 0;
        int viewport_height = 0;

        Color background_color = {255, 255, 255, 255};
    };
}




#endif //RENDERER_H
