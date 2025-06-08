//
// Created by victor on 08/06/25.
//

#ifndef COPY_SHADER_H
#define COPY_SHADER_H

inline const char* copy_frag = R""""(
#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoords) * vec4(1, 1, 0, 1);
}
)"""";

#endif //COPY_SHADER_H
