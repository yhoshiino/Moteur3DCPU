// ============================================================
//  color.h — packing colours and small colour helpers
// ------------------------------------------------------------
//  Shared by main.c (clear colour, normals shader...) and by the
//  shaders in material.h, which is why it lives in its own file.
// ============================================================
#pragma once
#include <stdint.h>

// Pack three 0..255 channels into one ARGB8888 pixel (chapter 1).
// Alpha is always 255 (opaque).
static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)0xFFu << 24)
        | ((uint32_t)r << 16)
        | ((uint32_t)g << 8)
        | (uint32_t)b;
}

// x^32 in five multiplications: x^2, x^4, x^8, x^16, then x^16 * x^16.
// Used for the specular highlight (shininess fixed at 32).
static inline float pow32(float x) {
    float x2 = x * x, x4 = x2 * x2, x8 = x4 * x4, x16 = x8 * x8;
    return x16 * x16;
}

// texel * diffuse brightness + white * specular, each channel
// clamped to 255 (chapter 11).
static inline uint32_t color_phong(uint32_t texel, float diffuse_k, float spec_k) {
    float r = (float)((texel >> 16) & 0xFF) * diffuse_k + 255.0f * spec_k;
    float g = (float)((texel >> 8) & 0xFF) * diffuse_k + 255.0f * spec_k;
    float b = (float)(texel & 0xFF) * diffuse_k + 255.0f * spec_k;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return ((uint32_t)0xFFu << 24)
        | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}