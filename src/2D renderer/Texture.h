#ifndef TEXTURE_H
#define TEXTURE_H

#include "../../libs/glad/include/glad/glad.h"
#include "../../libs/glm/glm.hpp"

namespace Odin
{
	struct TexInfo
	{
		/*
		* All the internal formats :
		*
		*  GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER,
		*  GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL
		*/
		GLuint internalFormat; // Number of color components in the texture

		/*
		* See https://www.khronos.org/opengl/wiki/Template:Glapi_view_format_map for more infos about format
		*/
		GLuint format; // Format of the pixel data ( GL_RGBA16F, ... )

		GLenum target; // Target the texture may be bind to ( GL_TEXTURE_2D, GL_TEXTURE_2D_MULTISAMPLE, ... )

		bool isMultisample;
		int samples;
		bool initialized = false;
		glm::vec2 dimensions;
	};

	class Texture
	{
	public:
		Texture();
		~Texture();

		void Init(GLuint internalFormat, GLuint format, int _width, int _height, int samples = 1);

		void Resize(int _width, int _height);

		void SetParameteri(GLenum pname, GLint param);
		void SetParameterf(GLenum pname, GLfloat param);

		void SetParameterfv(GLenum pname, const GLfloat* params);
		void SetParameteriv(GLenum pname, const GLint* params);
		void SetParameterIiv(GLenum pname, const GLint* params);
		void SetParameterIuiv(GLenum pname, const GLuint* params);

		GLenum GetTarget();
		unsigned int ID();

	private:
		unsigned int texID;
		TexInfo infos;
	};
}

#endif