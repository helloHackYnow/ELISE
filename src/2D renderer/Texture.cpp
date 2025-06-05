#include "Texture.h"

namespace Odin
{

	Texture::Texture()
	{
	}

	Texture::~Texture()
	{
		if (this->infos.initialized)
		{
			glDeleteTextures(1, &texID);
		}
	}

	void Texture::Init(GLuint internalFormat, GLuint format, int _width, int _height, int samples)
	{
		infos.format			= format;
		infos.internalFormat	= internalFormat;
		infos.isMultisample		= samples > 1;
		infos.samples			= samples;
		infos.target			= samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; // If msaa : GL_TEXTURE_2D_MULTISAMPLE, else GL_TEXTURE_2D
		infos.dimensions		= glm::vec2(_width, _height);

		glGenTextures(1, &texID);

		if (infos.isMultisample)
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texID);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, infos.samples, infos.format, _width, _height, GL_FALSE);
		}
		else
		{
			glBindTexture(infos.target, texID);
			glTexImage2D(GL_TEXTURE_2D, 0, infos.format, _width, _height, 0, infos.internalFormat, GL_FLOAT, nullptr);

		}

		glBindTexture(infos.target, 0); // Cleanup
		infos.initialized = true;
	}

	void Texture::Resize(int _width, int _height)
	{
		if (infos.isMultisample)
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texID);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, infos.samples, infos.format, _width, _height, GL_FALSE);
		}
		else
		{
			glBindTexture(infos.target, texID);
			glTexImage2D(infos.target, 0, infos.format, _width, _height, 0, infos.internalFormat, GL_FLOAT, nullptr);
		}
		

		infos.dimensions = glm::vec2(_width, _height);
		glBindTexture(infos.target, 0); // Cleanup
	}

	void Texture::SetParameteri(GLenum pname, GLint param)
	{
		glTexParameteri(texID, pname, param);
	}

	void Texture::SetParameterf(GLenum pname, GLfloat param)
	{
		glTexParameterf(texID, pname, param);
	}

	void Texture::SetParameterfv(GLenum pname, const GLfloat* params)
	{
		glTexParameterfv(texID, pname, params);
	}

	void Texture::SetParameteriv(GLenum pname, const GLint* params)
	{
		glTexParameteriv(texID, pname, params);
	}

	void Texture::SetParameterIiv(GLenum pname, const GLint* params)
	{
		glTexParameterIiv(texID, pname, params);
	}

	void Texture::SetParameterIuiv(GLenum pname, const GLuint* params)
	{
		glTexParameterIuiv(texID, pname, params);
	}

	GLenum Texture::GetTarget()
	{
		return infos.target;
	}

	unsigned int Texture::ID()
	{
		return texID;
	}

} //Pyewton::Renderer