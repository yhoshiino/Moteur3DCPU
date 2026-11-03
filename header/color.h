#pragma once
#include <stdint.h>

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)0xFFu << 24)
        | ((uint32_t)r << 16)
        | ((uint32_t)g << 8)
        | (uint32_t)b;
}

static inline float pow32(float x) {
    float x2 = x * x, x4 = x2 * x2, x8 = x4 * x4, x16 = x8 * x8;
    return x16 * x16;
}

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

static inline uint32_t color_blend(uint32_t src, uint32_t dst, uint8_t alpha) {
    float a = (float)alpha / 255.0f;
    float r = (float)((src >> 16) & 0xFF) * a + (float)((dst >> 16) & 0xFF) * (1.0f - a);
    float g = (float)((src >> 8) & 0xFF) * a + (float)((dst >> 8) & 0xFF) * (1.0f - a);
    float b = (float)(src & 0xFF) * a + (float)(dst & 0xFF) * (1.0f - a);
    return rgb((uint8_t)r, (uint8_t)g, (uint8_t)b);
}