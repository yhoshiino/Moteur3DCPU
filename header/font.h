#pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4244 4456 4457 4459 4505 4267 4701)
#endif
#include "stb_truetype.h"
#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#define FONT_FIRST_CHAR 32     // espace
#define FONT_CHAR_COUNT 95     // ' ' .. '~'
#define FONT_ATLAS_SIZE 512    // atlas carré en texels


struct Font {
    std::vector<uint8_t> atlas;
    stbtt_bakedchar glyphs[FONT_CHAR_COUNT] = {};

    bool load(const std::string& path, float pixelHeight);
    int  textWidth(std::string_view text) const;
    void drawText(uint32_t* fb, int fbW, int fbH, int x, int y,
        std::string_view text, uint32_t color) const;
};

namespace font_detail {
    inline uint32_t blend(uint32_t dst, uint32_t src, int a) {
        int ia = 255 - a;
        uint32_t r = (((src >> 16) * a + (dst >> 16) * a) * ia) / 255;
        uint32_t g = (((src >> 16) * a + (dst >> 16) * a) * ia) / 255;
        uint32_t b = (((src >> 16) * a + (dst >> 16) * a) * ia) / 255;
        return (0xFFu << 24) | (r << 16) | (g << 8) | b;
    }
};

inline bool Font::load(const std::string& path, float pixelHeight) {
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        std::cerr << ("Fichier introuvable ou inconnu");
        return false;
    };

    std::vector<unsigned char> ttf((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    if (ttf.empty()) {
        std::cerr << "Font::load : not readable";
        return false;
    };

    atlas.assign(FONT_ATLAS_SIZE * FONT_ATLAS_SIZE, 0);
    int rows = stbtt_BakeFontBitmap(ttf.data(), 0, pixelHeight, atlas.data(), FONT_ATLAS_SIZE, FONT_ATLAS_SIZE, FONT_FIRST_CHAR, FONT_CHAR_COUNT, glyphs);

    if (rows <= 0) {
        std::cerr << "Font::load: atlas too small for size " << pixelHeight << "\n";
        atlas.clear();
        return false;
    }

};

inline int Font::textWidth(std::string_view text) const {
    float w = 0.0f;
    for (unsigned char c : text) {
        if (c < FONT_FIRST_CHAR || c >= FONT_FIRST_CHAR + FONT_CHAR_COUNT) continue;
        w += glyphs[c - FONT_FIRST_CHAR].xadvance;
    }
    return static_cast<int>(w);
}

inline void Font::drawText(uint32_t* fb, int fbW, int fbH, int x, int y,
    std::string_view text, uint32_t color) const {
    float penX = static_cast<float>(x);
    for (unsigned char c : text) {
        if (c < FONT_FIRST_CHAR || c >= FONT_FIRST_CHAR + FONT_CHAR_COUNT) continue;
        const stbtt_bakedchar& g = glyphs[c - FONT_FIRST_CHAR];

        int gw = g.x1 - g.x0;
        int gh = g.y1 - g.y0;
        int dx = static_cast<int>(penX + g.xoff + 0.5f);
        int dy = static_cast<int>(static_cast<float>(y) + g.yoff + 0.5f);

        for (int row = 0; row < gh; row++) {
            for (int col = 0; col < gw; col++) {
                int a = atlas[(g.y0 + row) * FONT_ATLAS_SIZE + (g.x0 + col)];
                if (a == 0) continue;
                int px = dx + col, py = dy + row;
                if (px < 0 || px >= fbW || py < 0 || py >= fbH) continue;
                uint32_t& dst = fb[py * fbW + px];
                dst = font_detail::blend(dst, color, a);
            }
        }
        penX += g.xadvance;
    }
}