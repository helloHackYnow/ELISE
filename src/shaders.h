//
// Created by victor on 26/05/25.
//

#ifndef SHADERS_H
#define SHADERS_H

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

inline const char* bright_frag = R""""(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D scene;
uniform float threshold = 1.0;

void main() {
    vec3 color = texture(scene, TexCoord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0);
}

)"""";

inline const char* bright_vert = R""""(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    TexCoord = aTexCoord;
}

)"""";

inline const char* blur_vert = R""""(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    TexCoord = aTexCoord;
}

)"""";

inline const char* blur_frag = R""""(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D image;
uniform bool horizontal;

const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0); // 1 / texture resolution
    vec3 result = texture(image, TexCoord).rgb * weight[0];
    for (int i = 1; i < 5; ++i) {
        if (horizontal) {
            result += texture(image, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        } else {
            result += texture(image, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}

)"""";

inline const char* combine_vert = R""""(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform float bloomIntensity = 1.0;

void main() {
    vec3 color = texture(scene, TexCoord).rgb;
    vec3 blur = texture(bloom, TexCoord).rgb;
    FragColor = vec4(color + blur * bloomIntensity, 1.0);
}

)"""";

inline const char* combine_frag = R""""(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform float bloomIntensity = 1.0;

void main() {
    vec3 color = texture(scene, TexCoord).rgb;
    vec3 blur = texture(bloom, TexCoord).rgb;
    FragColor = vec4(color + blur * bloomIntensity, 1.0);
}

)"""";


#endif //SHADERS_H
