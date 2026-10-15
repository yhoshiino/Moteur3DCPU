// ============================================================
//  Chapter 04 — Vectors and matrices: spinning triangle
// ------------------------------------------------------------
//  Exercises vec.h and mat.h by spinning an equilateral
//  triangle around the screen centre with a rotation matrix.
//  A faint reference circle is drawn around it: because rotation
//  preserves length, the corners stay exactly on the circle as
//  the triangle turns. If a matrix routine were wrong, the
//  corners would visibly drift off the circle.
// ============================================================

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "vec.h"
#include "mat.h"


#define WIDTH  800
#define HEIGHT 600
#define PI 3.14159265358979323846f

static uint32_t framebuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)0xFFu << 24) | ((uint32_t)r << 16)
        | ((uint32_t)g << 8) | (uint32_t)b;
}

static inline void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    framebuffer[y * WIDTH + x] = color;
}

static void clear(uint32_t color) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) framebuffer[i] = color;
}

// Bresenham line (chapter 2) and barycentric triangle (chapter 3),
// reused unchanged.
static void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static inline int edge(int ax, int ay, int bx, int by, int px, int py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

// Blend three corner colours by the barycentric weights (chapter 3).
static inline uint32_t shade(int e0, int e1, int e2, int wsum,
    uint32_t c0, uint32_t c1, uint32_t c2) {
    uint8_t r = (uint8_t)((e0 * ((c0 >> 16) & 0xFF)
        + e1 * ((c1 >> 16) & 0xFF)
        + e2 * ((c2 >> 16) & 0xFF)) / wsum);
    uint8_t g = (uint8_t)((e0 * ((c0 >> 8) & 0xFF)
        + e1 * ((c1 >> 8) & 0xFF)
        + e2 * ((c2 >> 8) & 0xFF)) / wsum);
    uint8_t b = (uint8_t)((e0 * (c0 & 0xFF)
        + e1 * (c1 & 0xFF)
        + e2 * (c2 & 0xFF)) / wsum);
    return rgb(r, g, b);
}

// Filled triangle with per-corner colours interpolated via
// barycentric coordinates (chapter 3). Each corner carries its own
// colour, blended smoothly across the interior.
static void draw_triangle(int x0, int y0, uint32_t c0,
    int x1, int y1, uint32_t c1,
    int x2, int y2, uint32_t c2) {
    int area = edge(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;
    if (area < 0) {
        int tx = x1; x1 = x2; x2 = tx;
        int ty = y1; y1 = y2; y2 = ty;
        uint32_t tc = c1; c1 = c2; c2 = tc;
        area = -area;
    }
    int minx = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int miny = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxx = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    int maxy = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= WIDTH)  maxx = WIDTH - 1;
    if (maxy >= HEIGHT) maxy = HEIGHT - 1;
    for (int y = miny; y <= maxy; y++)
        for (int x = minx; x <= maxx; x++) {
            int e0 = edge(x1, y1, x2, y2, x, y);
            int e1 = edge(x2, y2, x0, y0, x, y);
            int e2 = edge(x0, y0, x1, y1, x, y);
            if ((e0 | e1 | e2) >= 0)
                framebuffer[y * WIDTH + x] =
                shade(e0, e1, e2, area, c0, c1, c2);
        }
}

// Draw a circle outline with the integer midpoint circle algorithm
// (Appendix B.4): no sin, cos, or sqrt — just integer adds and the
// circle's 8-fold symmetry. We walk one eighth of the circle (from
// the top, x=0, to the 45-degree point where x==y) and plot all
// eight mirror-image pixels at each step. `d` is the decision
// variable, the same idea as Bresenham's line error term.
static void draw_circle_outline(int cx, int cy, int r, uint32_t color) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;
    while (x <= y) {
        // the eight symmetric points
        put_pixel(cx + x, cy + y, color);
        put_pixel(cx - x, cy + y, color);
        put_pixel(cx + x, cy - y, color);
        put_pixel(cx - x, cy - y, color);
        put_pixel(cx + y, cy + x, color);
        put_pixel(cx - y, cy + x, color);
        put_pixel(cx + y, cy - x, color);
        put_pixel(cx - y, cy - x, color);
        x++;
        if (d > 0) {
            y--;
            d += 4 * (x - y) + 10;
        }
        else {
            d += 4 * x + 6;
        }
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Chapter 04 — Vectors & matrices", WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    // An equilateral triangle defined around its own centre (the
    // origin), with corners on a circle of this radius. Three points
    // 120 degrees apart.
    const float R = 200.0f;
    vec3 base[3];
    for (int i = 0; i < 3; i++) {
        float a = (float)i / 3.0f * 2.0f * PI - PI / 2.0f;  // start at top
        base[i] = vec3_make(R * cosf(a), R * sinf(a), 0.0f);
    }

    // One colour per corner: red, green, blue. The rasterizer blends
    // them across the triangle (chapter 3's barycentric gradient).
    const uint32_t corner_color[3] = {
        rgb(255,  60,  60),   // top
        rgb(60, 255,  60),   // bottom-right
        rgb(60,  60, 255),   // bottom-left
    };

    // FPS measurement (chapter 2). From this chapter on, every demo
    // does real per-frame work, so the FPS / ms-per-frame number in
    // the title is meaningful — and the gap between C and Python is
    // dramatic.
    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 start = SDL_GetPerformanceCounter();
    Uint64 last = start;
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

        // Seconds since launch → rotation angle.
        double seconds = (double)(SDL_GetPerformanceCounter() - start) / (double)freq;
        float angle = (float)seconds;   // 1 radian per second

        // Build the transform: rotate around the centre, then move
        // to the middle of the screen. Rightmost matrix is applied
        // first, so this is "rotate, then translate".
        //
        // A positive angle spins the triangle CLOCKWISE here, because
        // the framebuffer is y-down (chapter 1). The rotation matrix
        // is the standard math one (counter-clockwise in y-up); the
        // y-down screen just flips the visual direction. See README
        // section 4.8.
        mat4 transform = mat4_mul(
            mat4_translation(WIDTH / 2.0f, HEIGHT / 2.0f, 0.0f),
            mat4_rotation_z(angle));

        // Apply the transform to each corner.
        vec3 p[3];
        for (int i = 0; i < 3; i++) p[i] = mat4_apply_point(transform, base[i]);

        clear(rgb(16, 16, 24));

        // Reference circle (faint), centred on screen.
        draw_circle_outline(WIDTH / 2, HEIGHT / 2, (int)R, rgb(60, 60, 80));

        // Filled triangle with an R/G/B corner gradient.
        draw_triangle((int)p[0].x, (int)p[0].y, corner_color[0],
            (int)p[1].x, (int)p[1].y, corner_color[1],
            (int)p[2].x, (int)p[2].y, corner_color[2]);

        // White outline so the corners on the circle are easy to see.
        draw_line((int)p[0].x, (int)p[0].y, (int)p[1].x, (int)p[1].y, rgb(255, 255, 255));
        draw_line((int)p[1].x, (int)p[1].y, (int)p[2].x, (int)p[2].y, rgb(255, 255, 255));
        draw_line((int)p[2].x, (int)p[2].y, (int)p[0].x, (int)p[0].y, rgb(255, 255, 255));

        SDL_UpdateTexture(texture, NULL, framebuffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Update the FPS counter once per second.
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / (double)freq;
        last = now;
        fps_accum += dt;
        fps_count += 1;
        if (fps_accum >= 1.0) {
            double fps = (double)fps_count / fps_accum;
            double ms = 1000.0 * fps_accum / (double)fps_count;
            snprintf(title, sizeof title,
                "Chapter 04 — Vectors & matrices  |  %.1f FPS  (%.2f ms/frame)",
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