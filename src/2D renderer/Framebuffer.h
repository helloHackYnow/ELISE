#ifndef RENDERER_FRAMEBUFFER_H_
#define RENDERER_FRAMEBUFFER_H_

#include "../../libs/glm/glm.hpp"
#include "Texture.h"

#include <iostream>
#include "../../libs/glad/include/glad/glad.h"

namespace Odin
{
	struct FBOInfos
	{
		unsigned int samples;
		glm::vec2 dimensions;
		bool isHDR;
	};

	class FrameBuffer
	{
	public:
		FrameBuffer();
		~FrameBuffer();

		void Init(int _width, int _height, bool isHDR = false, int samples = 1);
		void Rescale(int _width, int _height);
		void Use();

		unsigned int GetTexture();
		unsigned int ID();

		bool IsMultiSamples();

	private:
		glm::vec2 dimensions;

		Texture texture;

		unsigned int fboID;
		unsigned int rboID;

		int samples;

	};
}

#endif