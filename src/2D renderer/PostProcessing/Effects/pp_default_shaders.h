//
// Created by victor on 05/06/25.
//

#ifndef PP_DEFAULT_SHADERS_H
#define PP_DEFAULT_SHADERS_H

inline const char* pp_default_vertex = R""""(
#version 450 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoords = aTexCoords;
}
)"""";

inline const char* pp_default_fragment = R""""(
#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoords) * vec4(1, 1, 0, 1);
}
)"""";

#endif //PP_DEFAULT_SHADERS_H
