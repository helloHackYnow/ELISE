//
// Created by victor on 05/06/25.
//

#ifndef BLOOM_SHADERS_H
#define BLOOM_SHADERS_H

inline const char* bs_blur_frag = R""""(
#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 blur_dir;

const int SAMPLE_COUNT = 11;

const float OFFSETS[11] = float[11](
    -9.260003189282239,
    -7.304547036499911,
    -5.353083811756559,
    -3.4048471718931532,
    -1.4588111840004858,
    0.48624268466894843,
    2.431625915613778,
    4.378621204796657,
    6.328357272092126,
    8.281739853232981,
    10
);

const float WEIGHTS[11] = float[11](
    0.002071619848105582,
    0.012832728894200915,
    0.0517012035286156,
    0.1355841921107385,
    0.23159560769543552,
    0.2577662485651885,
    0.18695197035734282,
    0.08833722224378082,
    0.027179417353550506,
    0.005441161635553416,
    0.0005386277674878371
);

// blurDirection is:
//     vec2(1,0) for horizontal pass
//     vec2(0,1) for vertical pass
// The sourceTexture to be blurred MUST use linear filtering!
// pixelCoord is in [0..1]
vec4 blur(in sampler2D sourceTexture, vec2 blurDirection, vec2 pixelCoord)
{
    vec4 result = vec4(0.0);
    vec2 size = textureSize(sourceTexture, 0);
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec2 offset = blurDirection * OFFSETS[i] / size;
        float weight = WEIGHTS[i];
        result += texture(sourceTexture, pixelCoord + offset) * weight;
    }
    return result;
}

void main()
{
    FragColor = blur(screenTexture, blur_dir, TexCoords);
}
)"""";

inline const char* bs_combine_frag = R""""(
#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D bloomTex;
uniform sampler2D mainTex;

float bias = 0.2;

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec4 color = vec4(texture(mainTex, TexCoords) + texture(bloomTex, TexCoords) * bias);
    FragColor = vec4(aces(color.xyz), 1);
}
)"""";

inline const char* bs_downsample_frag = R""""(
#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

vec3 DownsampleBox13Tap(in sampler2D sourceTexture, vec2 uv, vec2 srcTexelSize)
{
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(sourceTexture, vec2(uv.x - 2*x  , uv.y + 2*y )).rgb;
    vec3 b = texture(sourceTexture, vec2(uv.x        , uv.y + 2*y )).rgb;
    vec3 c = texture(sourceTexture, vec2(uv.x + 2*x  , uv.y + 2*y )).rgb;
    vec3 d = texture(sourceTexture, vec2(uv.x - 2*x  , uv.y       )).rgb;
    vec3 e = texture(sourceTexture, vec2(uv.x        , uv.y       )).rgb;
    vec3 f = texture(sourceTexture, vec2(uv.x + 2*x  , uv.y       )).rgb;
    vec3 g = texture(sourceTexture, vec2(uv.x - 2*x  , uv.y - 2*y )).rgb;
    vec3 h = texture(sourceTexture, vec2(uv.x        , uv.y - 2*y )).rgb;
    vec3 i = texture(sourceTexture, vec2(uv.x + 2*x  , uv.y - 2*y )).rgb;
    vec3 j = texture(sourceTexture, vec2(uv.x - x    , uv.y + y   )).rgb;
    vec3 k = texture(sourceTexture, vec2(uv.x + x    , uv.y + y   )).rgb;
    vec3 l = texture(sourceTexture, vec2(uv.x - x    , uv.y - y   )).rgb;
    vec3 m = texture(sourceTexture, vec2(uv.x + x    , uv.y - y   )).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    vec3 downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    return downsample;
}

void main()
{
    vec2 srcTexelSize = 1.0 / textureSize(screenTexture, 0);
    FragColor = vec4(DownsampleBox13Tap(screenTexture, TexCoords, srcTexelSize), 1);
}
)"""";

inline const char* bs_prefilter_frag = R""""(
#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float exposure;
uniform float knee;
uniform float threshold;

//https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.high-definition/Runtime/PostProcessing/Shaders/BloomCommon.hlsl 05 august 2023
// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
//vec4 threshold = x: threshold value (linear), y: threshold - knee, z: knee * 2, w: 0.25 / knee
vec3 QuadraticThreshold(vec3 color, float threshold_, vec3 curve)
{
    // Pixel brightness
    float br = max(color.r, max(color.g, color.b));

    // Under-threshold part
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve
    color *= max(rq, br - threshold_) / max(br, 1e-4);

    return color;
}

vec4 Prefilter(vec4 color, vec4 threshold)
{
    color = vec4(QuadraticThreshold(color.xyz,  threshold.x, threshold.yzw), 1);
    return color;
}

void main()
{
    vec4 threshold_ = vec4(threshold, threshold - knee, knee * 2, 0.25 / knee);
    vec4 color = texture(screenTexture, TexCoords);
    FragColor =  Prefilter(color, threshold_);
}
)"""";

inline const char* bs_tonesmap_frag = R""""(
)"""";

inline const char* bs_upsample_frag = R""""(
#version 450 core

// This shader performs upsampling on a texture,
// as taken from Call Of Duty method, presented at ACM Siggraph 2014.

// Remember to add bilinear minification filter for this texture!
// Remember to use a floating-point texture format (for HDR)!
// Remember to use edge clamping for this texture!
uniform sampler2D srcTexture;
uniform float filterRadius;

in vec2 TexCoords;
layout (location = 0) out vec3 upsample;

void main()
{
    // The filter kernel is applied with a radius, specified in texture
    // coordinates, so that the radius will vary across mip resolutions.
    float x = filterRadius;
    float y = filterRadius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

    vec3 d = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y)).rgb;
    vec3 e = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y)).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y)).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    upsample = e*4.0;
    upsample += (b+d+f+h)*2.0;
    upsample += (a+c+g+i);
    upsample *= 1.0 / 16.0;
}
)"""";

#endif //BLOOM_SHADERS_H
