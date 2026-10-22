// ============================================================
//  texture.h — load a PNG and sample it (chapter 9)
// ------------------------------------------------------------
//  We decode the PNG with stb_image, a single-header public-domain
//  library (third-party/stb_image.h), then repack its RGBA bytes
//  into our ARGB8888 uint32_t layout so a sampled texel can be
//  written straight into the framebuffer.
//
//  Two convention notes for the sampler:
//    - OBJ 'vt' coordinates put v = 0 at the BOTTOM of the image,
//      but image rows (and stb_image) start at the TOP — so we
//      flip v when indexing.
//    - U and V are wrapped into [0, 1] (fractional part) so a
//      stray out-of-range coordinate can't read past the array.
//
//  Sampling here is nearest-neighbour: pick the single closest
//  texel. It can look blocky up close; chapter 12 adds the
//  smoother bilinear version.
// ============================================================
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// stb_image is general-purpose and noisier under /W4 than our own
// code, so we quiet the warnings just around its implementation.
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
    uint32_t* pixels;     // width * height texels, ARGB8888
    int       width;
    int       height;
} Texture;

// Load a PNG into ARGB8888. Returns 1 on success, 0 on failure.
static int texture_load(const char* path, Texture* out) {
    int w, h, channels;
    // Ask stb_image for 4 channels; it fills alpha = 255 for images
    // that don't carry one.
    uint8_t* data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) {
        fprintf(stderr, "texture_load: failed on '%s': %s\n",
            path, stbi_failure_reason());
        return 0;
    }

    out->width = w;
    out->height = h;
    out->pixels = (uint32_t*)malloc((size_t)w * (size_t)h * sizeof(uint32_t));

    // Repack RGBA8 -> ARGB8888 (our framebuffer's native layout).
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

// Nearest-neighbour sample at texture coordinate (u, v). U and V are
// wrapped to [0, 1]; v is flipped because OBJ's v = 0 is the image
// bottom but our pixel array's row 0 is the top.
static inline uint32_t texture_sample(const Texture* tex, float u, float v) {
    u = u - floorf(u);
    v = v - floorf(v);
    int x = (int)(u * (float)tex->width);
    int y = (int)((1.0f - v) * (float)tex->height);
    if (x < 0) x = 0; if (x >= tex->width)  x = tex->width - 1;
    if (y < 0) y = 0; if (y >= tex->height) y = tex->height - 1;
    return tex->pixels[y * tex->width + x];
}

// Bilinear sample (chapter 12). Instead of snapping to one texel, we
// fetch the FOUR nearest and blend them by how far the exact UV sits
// between them. This smooths the blocky stair-steps nearest-neighbour
// gives when the texture is magnified (camera close to the surface).
//
// The four corner weights sum to 1:
//     w00 = (1-tx)(1-ty)   w10 = tx(1-ty)
//     w01 = (1-tx)·ty      w11 = tx·ty
static inline uint32_t texture_sample_bilinear(const Texture* tex, float u, float v) {
    u = u - floorf(u);
    v = v - floorf(v);

    // Continuous texel position. The (size-1) keeps u=v=1 landing on
    // the last texel instead of one past the edge.
    float fx = u * (float)(tex->width - 1);
    float fy = (1.0f - v) * (float)(tex->height - 1);

    int   x0 = (int)floorf(fx);
    int   y0 = (int)floorf(fy);
    int   x1 = x0 + 1;
    int   y1 = y0 + 1;
    float tx = fx - (float)x0;     // fractional position inside the cell
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