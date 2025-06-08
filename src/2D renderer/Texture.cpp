#include "Texture.h"

namespace Odin
{

    Texture::Texture() : texID(0)
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
        infos.format            = format;
        infos.internalFormat    = internalFormat;
        infos.isMultisample     = samples > 1;
        infos.samples           = samples;
        infos.target            = samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; // If msaa : GL_TEXTURE_2D_MULTISAMPLE, else GL_TEXTURE_2D
        infos.dimensions        = glm::vec2(_width, _height);

        glGenTextures(1, &texID);

        if (infos.isMultisample)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texID);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, infos.samples, infos.internalFormat, _width, _height, GL_FALSE);
        }
        else
        {
            glBindTexture(infos.target, texID);
            glTexImage2D(GL_TEXTURE_2D, 0, infos.internalFormat, _width, _height, 0, infos.format, GL_FLOAT, nullptr);
        }

        glBindTexture(infos.target, 0); // Cleanup
        infos.initialized = true;
    }

    void Texture::Resize(int _width, int _height)
    {
        if (infos.isMultisample)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texID);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, infos.samples, infos.internalFormat, _width, _height, GL_FALSE);
        }
        else
        {
            glBindTexture(infos.target, texID);
            glTexImage2D(infos.target, 0, infos.internalFormat, _width, _height, 0, infos.format, GL_FLOAT, nullptr);
        }
        
        infos.dimensions = glm::vec2(_width, _height);
        glBindTexture(infos.target, 0); // Cleanup
    }

    void Texture::SetParameteri(GLenum pname, GLint param)
    {
        if (infos.isMultisample) return; // Multisample textures don't support parameters
        
        glBindTexture(infos.target, texID);
        glTexParameteri(infos.target, pname, param);
        glBindTexture(infos.target, 0);
    }

    void Texture::SetParameterf(GLenum pname, GLfloat param)
    {
        if (infos.isMultisample) return;
        
        glBindTexture(infos.target, texID);
        glTexParameterf(infos.target, pname, param);
        glBindTexture(infos.target, 0);
    }

    void Texture::SetParameterfv(GLenum pname, const GLfloat* params)
    {
        if (infos.isMultisample) return;
        
        glBindTexture(infos.target, texID);
        glTexParameterfv(infos.target, pname, params);
        glBindTexture(infos.target, 0);
    }

    void Texture::SetParameteriv(GLenum pname, const GLint* params)
    {
        if (infos.isMultisample) return;
        
        glBindTexture(infos.target, texID);
        glTexParameteriv(infos.target, pname, params);
        glBindTexture(infos.target, 0);
    }

    void Texture::SetParameterIiv(GLenum pname, const GLint* params)
    {
        if (infos.isMultisample) return;
        
        glBindTexture(infos.target, texID);
        glTexParameterIiv(infos.target, pname, params);
        glBindTexture(infos.target, 0);
    }

    void Texture::SetParameterIuiv(GLenum pname, const GLuint* params)
    {
        if (infos.isMultisample) return;
        
        glBindTexture(infos.target, texID);
        glTexParameterIuiv(infos.target, pname, params);
        glBindTexture(infos.target, 0);
    }

    GLenum Texture::GetTarget()
    {
        return infos.target;
    }

    unsigned int Texture::ID()
    {
        return texID;
    }

} //Odin