#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

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

struct Texture {
    std::vector<uint32_t> pixels;
    int width = 0;
    int height = 0;

    bool load(const std::string& path);
    uint32_t sample(float u, float v) const;
    uint32_t sampleBilinear(float u, float v) const;
};

namespace tex_detail {

    inline float wrap01(float x) { return x - std::floor(x); }

    inline uint32_t lerp8(uint32_t a, uint32_t b, uint32_t t8) {
        uint32_t s = 256 - t8;
        uint32_t rb = (((a & 0x00FF00FFu) * s + (b & 0x00FF00FFu) * t8) >> 8) & 0x00FF00FFu;
        uint32_t g = (((a & 0x0000FF00u) * s + (b & 0x0000FF00u) * t8) >> 8) & 0x0000FF00u;
        return rb | g;
    }

}

inline bool Texture::load(const std::string& path) {
    int w, h, channels;
    std::unique_ptr<stbi_uc, void (*)(void*)> data(
        stbi_load(path.c_str(), &w, &h, &channels, 4), stbi_image_free);
    if (!data) {
        std::cerr << "Texture::load: failed on '" << path << "': " << stbi_failure_reason() << "\n";
        return false;
    }

    width = w;
    height = h;
    pixels.resize(static_cast<size_t>(w) * h);

    const stbi_uc* p = data.get();
    for (uint32_t& px : pixels) {
        px = uint32_t(p[3]) << 24 | uint32_t(p[0]) << 16 | uint32_t(p[1]) << 8 | uint32_t(p[2]);
        p += 4;
    }
    return true;
}

inline uint32_t Texture::sample(float u, float v) const {
    int x = std::min(static_cast<int>(tex_detail::wrap01(u) * width), width - 1);
    int y = std::min(static_cast<int>((1.0f - tex_detail::wrap01(v)) * height), height - 1);
    return pixels[y * width + x];
}

inline uint32_t Texture::sampleBilinear(float u, float v) const {
    float fx = tex_detail::wrap01(u) * (width - 1);
    float fy = (1.0f - tex_detail::wrap01(v)) * (height - 1);

    int x0 = static_cast<int>(fx);
    int y0 = static_cast<int>(fy);
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);

    uint32_t tx = static_cast<uint32_t>((fx - x0) * 256.0f);
    uint32_t ty = static_cast<uint32_t>((fy - y0) * 256.0f);

    const uint32_t* row0 = &pixels[y0 * width];
    const uint32_t* row1 = &pixels[y1 * width];
    uint32_t top = tex_detail::lerp8(row0[x0], row0[x1], tx);
    uint32_t bottom = tex_detail::lerp8(row1[x0], row1[x1], tx);
    return 0xFF000000u | tex_detail::lerp8(top, bottom, ty);
}