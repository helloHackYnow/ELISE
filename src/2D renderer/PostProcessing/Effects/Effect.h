#ifndef POST_PROCESSING_EFFECT_H
#define POST_PROCESSING_EFFECT_H

#include <memory>

#include "../../libs/glad/include/glad/glad.h"
#include <vector>
#include "../../Shader/Graphics.h"
#include "../../Framebuffer.h"
#include "pp_default_shaders.h"

namespace Odin
{
	class Effect
	{
	public:

		Effect();

		void InitVAO();

		// Return the index of the shader in the shaders vector
		virtual void Init();

		virtual void Apply(FrameBuffer& fbo_in, FrameBuffer& fbo_out, int w_width, int w_height);

	public:
		int AddShader(const char* fragment_data);

		unsigned int VAO;
		unsigned int VBO;

		std::vector<std::unique_ptr<FrameBuffer>> fbos;
		std::vector<Shader> shaders;

		// For keeping trace of the dimensions

		int width;
		int height;
	};
}

#endif