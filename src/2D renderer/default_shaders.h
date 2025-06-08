//
// Created by victor on 05/06/25.
//

#ifndef DEFAULT_SHADERS_H
#define DEFAULT_SHADERS_H

inline const char* light_vert = R""""(
#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uPosition;
uniform float uScale;

void main() {
    vec2 scaled = aPos * uScale;
    gl_Position = vec4(scaled + uPosition, 0.0, 1.0);
}
)"""";

inline const char* light_frag = R""""(
#version 330 core
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    FragColor = uColor;
}
)"""";

#endif //DEFAULT_SHADERS_H
