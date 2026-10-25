// ============================================================
//  font.h — draw text into the CPU framebuffer
// ------------------------------------------------------------
//  stb_truetype rasterises the printable ASCII glyphs of a .ttf
//  ONCE into an 8-bit alpha atlas (font_load). font_draw_text
//  then copies each glyph's pixels into the ARGB framebuffer,
//  blending by the glyph's alpha so the edges are smooth.
// ============================================================
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

typedef struct {
    uint8_t* atlas;                       // SIZE*SIZE, 1 octet d'alpha par texel
    stbtt_bakedchar glyphs[FONT_CHAR_COUNT];     // position/taille de chaque glyphe
} Font;

// Charge un .ttf et le rasterise à pixel_height pixels. 1 = succès, 0 = échec.
static int font_load(const char* path, float pixel_height, Font* out) {
    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "font_load: could not open %s\n", path); return 0; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* ttf = (unsigned char*)malloc((size_t)size);
    if (!ttf || fread(ttf, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "font_load: read error on %s\n", path);
        free(ttf); fclose(f); return 0;
    }
    fclose(f);

    out->atlas = (uint8_t*)malloc(FONT_ATLAS_SIZE * FONT_ATLAS_SIZE);
    int rows = stbtt_BakeFontBitmap(ttf, 0, pixel_height,
        out->atlas, FONT_ATLAS_SIZE, FONT_ATLAS_SIZE,
        FONT_FIRST_CHAR, FONT_CHAR_COUNT, out->glyphs);
    free(ttf);                       // l'atlas contient tout, le .ttf n'est plus utile

    if (rows <= 0) {                 // <= 0 : les glyphes ne tiennent pas dans l'atlas
        fprintf(stderr, "font_load: atlas too small for size %.0f\n", pixel_height);
        free(out->atlas); out->atlas = NULL; return 0;
    }
    return 1;
}

static void font_free(Font* font) {
    free(font->atlas);
    font->atlas = NULL;
}

// Mélange src sur dst avec une opacité a (0..255), canal par canal.
static inline uint32_t font__blend(uint32_t dst, uint32_t src, int a) {
    int ia = 255 - a;
    uint32_t r = (((src >> 16) & 0xFF) * a + ((dst >> 16) & 0xFF) * ia) / 255;
    uint32_t g = (((src >> 8) & 0xFF) * a + ((dst >> 8) & 0xFF) * ia) / 255;
    uint32_t b = ((src & 0xFF) * a + (dst & 0xFF) * ia) / 255;
    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// Dessine `text` dans fb (fb_w x fb_h). (x, y) = début de la LIGNE DE BASE.
static void font_draw_text(uint32_t* fb, int fb_w, int fb_h, const Font* font,
    int x, int y, const char* text, uint32_t color) {
    float pen_x = (float)x;
    for (const char* c = text; *c; c++) {
        if (*c < FONT_FIRST_CHAR || *c >= FONT_FIRST_CHAR + FONT_CHAR_COUNT) continue;
        const stbtt_bakedchar* g = &font->glyphs[*c - FONT_FIRST_CHAR];

        int gw = g->x1 - g->x0;
        int gh = g->y1 - g->y0;
        int dx = (int)(pen_x + g->xoff + 0.5f);   // coin haut-gauche du glyphe
        int dy = (int)((float)y + g->yoff + 0.5f);

        for (int row = 0; row < gh; row++) {
            for (int col = 0; col < gw; col++) {
                int a = font->atlas[(g->y0 + row) * FONT_ATLAS_SIZE + (g->x0 + col)];
                if (a == 0) continue;
                int px = dx + col, py = dy + row;
                if (px < 0 || px >= fb_w || py < 0 || py >= fb_h) continue;
                uint32_t* dst = &fb[py * fb_w + px];
                *dst = font__blend(*dst, color, a);
            }
        }
        pen_x += g->xadvance;                      // avance jusqu'au caractère suivant
    }
}