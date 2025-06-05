#ifndef SHADER_H
#define SHADER_H


#include "../../../libs/glm/glm.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "../../../libs/glad/include/glad/glad.h" // include glad to get all the required OpenGL headers

enum ShaderFlags
{
    shaderFlags_None            = 0,

    //If set, all infos concerning light points will be passed to the shader by uniforms
    shaderFlags_NeedLightPoints     = 1 << 0,
    //If set, all infos concerning ambient light will be passed to the shader by uniforms
    shaderFlags_NeedAmbientLight    = 1 << 1,

    //If set, the projection and the view matrices will be passed to the shader by uniforms
    shaderFlags_NeedCameraInfo      = 1 << 2,

    //If set, the exposure will be passed to the shader
    shaderFlags_NeedExposure        = 1 << 3,
};

class Shader
{
public:
    // the program ID
    unsigned int ID;
    int flags;

    // constructor reads and builds the shader
    Shader();
    // use/activate the shader
    void use();
    // utility uniform functions
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setMat2(const std::string& name, const glm::mat2& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;

protected:
    void checkCompileErrors(unsigned int shader, std::string type);
};


#endif