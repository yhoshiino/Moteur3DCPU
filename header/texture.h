#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ONLY_PNG
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4244 4456 4457 4459 4505 4267)
#endif
#include "stb_image.h"
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

typedef struct {
    uint32_t* pixels;
    int       width;
    int       height;
} Texture;

static int texture_load(const char* path, Texture* out) {
    int w, h, channels;

    uint8_t* data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) {
        fprintf(stderr, "texture_load: failed on '%s': %s\n",
            path, stbi_failure_reason());
        return 0;
    }

    out->width = w;
    out->height = h;
    out->pixels = (uint32_t*)malloc((size_t)w * (size_t)h * sizeof(uint32_t));

    for (int i = 0; i < w * h; i++) {
        uint8_t r = data[i * 4 + 0];
        uint8_t g = data[i * 4 + 1];
        uint8_t b = data[i * 4 + 2];
        uint8_t a = data[i * 4 + 3];
        out->pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16)
            | ((uint32_t)g << 8) | (uint32_t)b;
    }

    stbi_image_free(data);
    return 1;
}

static void texture_free(Texture* tex) {
    free(tex->pixels);
    tex->pixels = NULL;
    tex->width = tex->height = 0;
}

static inline uint32_t texture_sample(const Texture* tex, float u, float v) {
    u = u - floorf(u);
    v = v - floorf(v);
    int x = (int)(u * (float)tex->width);
    int y = (int)((1.0f - v) * (float)tex->height);
    if (x < 0) x = 0; if (x >= tex->width)  x = tex->width - 1;
    if (y < 0) y = 0; if (y >= tex->height) y = tex->height - 1;
    return tex->pixels[y * tex->width + x];
}

static inline uint32_t texture_sample_bilinear(const Texture* tex, float u, float v) {
    u = u - floorf(u);
    v = v - floorf(v);

    float fx = u * (float)(tex->width - 1);
    float fy = (1.0f - v) * (float)(tex->height - 1);

    int   x0 = (int)floorf(fx);
    int   y0 = (int)floorf(fy);
    int   x1 = x0 + 1;
    int   y1 = y0 + 1;
    float tx = fx - (float)x0;
    float ty = fy - (float)y0;

    if (x0 < 0) x0 = 0; if (x0 >= tex->width)  x0 = tex->width - 1;
    if (y0 < 0) y0 = 0; if (y0 >= tex->height) y0 = tex->height - 1;
    if (x1 < 0) x1 = 0; if (x1 >= tex->width)  x1 = tex->width - 1;
    if (y1 < 0) y1 = 0; if (y1 >= tex->height) y1 = tex->height - 1;

    uint32_t c00 = tex->pixels[y0 * tex->width + x0];
    uint32_t c10 = tex->pixels[y0 * tex->width + x1];
    uint32_t c01 = tex->pixels[y1 * tex->width + x0];
    uint32_t c11 = tex->pixels[y1 * tex->width + x1];

    float w00 = (1.0f - tx) * (1.0f - ty);
    float w10 = tx * (1.0f - ty);
    float w01 = (1.0f - tx) * ty;
    float w11 = tx * ty;

    float r = w00 * ((c00 >> 16) & 0xFF) + w10 * ((c10 >> 16) & 0xFF)
        + w01 * ((c01 >> 16) & 0xFF) + w11 * ((c11 >> 16) & 0xFF);
    float g = w00 * ((c00 >> 8) & 0xFF) + w10 * ((c10 >> 8) & 0xFF)
        + w01 * ((c01 >> 8) & 0xFF) + w11 * ((c11 >> 8) & 0xFF);
    float b = w00 * (c00 & 0xFF) + w10 * (c10 & 0xFF)
        + w01 * (c01 & 0xFF) + w11 * (c11 & 0xFF);

    return ((uint32_t)0xFFu << 24)
        | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}