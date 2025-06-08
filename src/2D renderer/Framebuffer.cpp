#include "Framebuffer.h"
/*
* Abstaction layer for handling frambuffer objects
*/


namespace Odin {

	FrameBuffer::FrameBuffer()
	{

	}

	FrameBuffer::~FrameBuffer()
	{

		glDeleteFramebuffers(1, &fboID);
		glDeleteRenderbuffers(1, &rboID);
	}
	
	void FrameBuffer::Init(int _width, int _height, bool isHDR, int samples)
	{
		dimensions = glm::vec2(_width, _height);

		GLuint internalFormat = GL_RGBA8;
		GLenum format = GL_RGBA;

		if (isHDR) {
			internalFormat = GL_R11F_G11F_B10F;
			format = GL_RGB;

		}

		glGenFramebuffers(1, &fboID);
		glBindFramebuffer(GL_FRAMEBUFFER, fboID);

		texture.Init(internalFormat, format, _width, _height, samples);

		texture.SetParameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture.SetParameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		texture.SetParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		texture.SetParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.GetTarget(), texture.ID(), 0);

		glGenRenderbuffers(1, &rboID);
		glBindRenderbuffer(GL_RENDERBUFFER, rboID);

		if (samples > 1) // Check if msaa
		{
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, _width, _height);
		}
		else
		{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
		}

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboID);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		this->samples = samples;
	}

	void FrameBuffer::Rescale(int _width, int _height)
	{
		if (_width == dimensions.x && _height == dimensions.y) return;

		dimensions = glm::vec2(_width, _height);

		texture.Resize(_width, _height);

		texture.SetParameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture.SetParameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		texture.SetParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		texture.SetParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		if (samples > 1)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, rboID);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, _width, _height);
		}
		else
		{
			glBindRenderbuffer(GL_RENDERBUFFER, rboID);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void FrameBuffer::Use()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fboID);
	}

	unsigned int FrameBuffer::GetTexture()
	{
		return texture.ID();
	}

	unsigned int FrameBuffer::ID()
	{
		return fboID;
	}

	bool FrameBuffer::IsMultiSamples()
	{
		return samples > 1;
	}

}