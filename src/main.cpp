// ============================================================
//  Chapter 02 — Bresenham line + FPS counter
// ------------------------------------------------------------
//  Adds the second primitive on top of put_pixel: a straight
//  line drawn with the 8-octant generalised Bresenham algorithm
//  (integer-only, no division or multiplication in the inner
//  loop). Also adds a per-second FPS meter shown in the window
//  title.
//
//  Math recap (see chapter README for the derivation):
//      Octant 0 only:
//          E_initial = 2*dy - dx
//          if (E > 0): y += 1; E += 2*(dy - dx)
//          else:                E += 2*dy
//      The generalised version below handles all 8 octants by
//      carrying sign variables sx, sy and a combined error.
// ============================================================

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define WIDTH  800
#define HEIGHT 600

static uint32_t framebuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)0xFFu << 24) | ((uint32_t)r << 16)
        | ((uint32_t)g << 8) | (uint32_t)b;
}

static inline void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    framebuffer[y * WIDTH + x] = color;
}

// Fill the framebuffer with a solid colour. Used to clear between
// frames so leftover pixels don't smear.
static void clear(uint32_t color) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) framebuffer[i] = color;
}

// Bresenham's line, generalised to all 8 octants.
// See the chapter README, step 5, for what each line is doing.
//
// All arithmetic is integer. No multiplication or division inside
// the loop - only adds and a sign test.
static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);             // negative on purpose
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;                   // = dx - |dy|

    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }   // step in x
        if (e2 <= dx) { err += dx; y0 += sy; }   // step in y
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Chapter 02 — Lines",
        WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        WIDTH, HEIGHT);

    // ---- FPS measurement state ----
    // SDL_GetPerformanceCounter ticks a high-resolution timer.
    // SDL_GetPerformanceFrequency tells us ticks per second.
    // So (now - last) / freq is the elapsed seconds.
    const Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();
    double fps_accum = 0.0;
    int    fps_count = 0;
    char   title[160];

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
                running = false;
        }

        // ---- Draw the frame ----
        clear(rgb(16, 16, 24));

        // A "fan" of lines from the centre. Each spoke goes in a
        // different direction. With 64 spokes covering all 8
        // octants, this visually proves draw_line handles every
        // octant correctly.
        const int cx = WIDTH / 2;
        const int cy = HEIGHT / 2;
        const int spokes = 64;
        const int radius = (HEIGHT / 2) - 20;
        const double TWO_PI = 6.2831853071795864;

        for (int i = 0; i < spokes; i++) {
            double t = (double)i / (double)spokes * TWO_PI;
            double ct = cos(t);
            double st = sin(t);
            int ex = cx + (int)(radius * ct);
            int ey = cy + (int)(radius * st);
            uint8_t r = (uint8_t)(127.0 + 127.0 * ct);
            uint8_t g = (uint8_t)(127.0 + 127.0 * st);
            draw_line(cx, cy, ex, ey, rgb(r, g, 200));
        }

        // ---- Ship the framebuffer to the screen ----
        SDL_UpdateTexture(texture, NULL, framebuffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // ---- Update the FPS counter ----
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / (double)freq;
        last = now;
        fps_accum += dt;
        fps_count += 1;
        if (fps_accum >= 1.0) {
            double fps = (double)fps_count / fps_accum;
            double ms = 1000.0 * fps_accum / (double)fps_count;
            snprintf(title, sizeof title,
                "Chapter 02 — Lines  |  %.1f FPS  (%.2f ms/frame)",
                fps, ms);
            SDL_SetWindowTitle(window, title);
            fps_accum = 0.0;
            fps_count = 0;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}