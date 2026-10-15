// ============================================================
//  Chapter 05 — The 3D pipeline: spinning wireframe cube
// ------------------------------------------------------------
//  First real 3D. A unit cube defined in model space is run
//  through MVP = Projection * View * Model, then each vertex is
//  divided by w (the perspective divide) and remapped to a pixel
//  (the viewport transform). The 12 edges are drawn with the
//  Bresenham line from chapter 2, colour-coded by axis.
//
//  Watch the near edges grow larger than the far ones — that is
//  perspective, produced by the divide-by-w.
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

// The 8 corners of a unit cube, centred on the origin (each
// coordinate is +1 or -1).
static const vec3 CUBE_VERTS[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1},
};

// The 12 edges, as pairs of corner indices, plus an axis tag
// (0 = runs along x, 1 = along y, 2 = along z) used to colour them.
static const int CUBE_EDGES[12][3] = {
    {0,1,0}, {3,2,0}, {7,6,0}, {4,5,0},   // edges along x
    {0,3,1}, {1,2,1}, {5,6,1}, {4,7,1},   // edges along y
    {0,4,2}, {1,5,2}, {2,6,2}, {3,7,2},   // edges along z
};

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    // Edge colour per axis (rgb() isn't a compile-time constant, so
    // we fill this at runtime rather than as a static initializer).
    uint32_t axis_color[3];
    axis_color[0] = rgb(230, 80, 80);   // x : red
    axis_color[1] = rgb(80, 210, 90);   // y : green
    axis_color[2] = rgb(90, 140, 240);   // z : blue

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Chapter 05 — 3D pipeline", WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    // Projection and view stay fixed; only the model spins.
    const float aspect = (float)WIDTH / (float)HEIGHT;

    // 60-degree vertical field of view (radians() converts to the
    // radians the math needs); window shape; near and far clip
    // distances. See README section 5.6.
    const mat4 projection = mat4_perspective(radians(60.0f), aspect,
        0.1f, 100.0f);

    // Camera: positioned 5 units back on +z (eye), aimed at the
    // origin (target), with world-up being +y. See README section 5.10.
    const vec3 eye = vec3_make(5, 0, 5);
    const vec3 target = vec3_make(0, 0, 0);
    const vec3 up = vec3_make(0, 1, 0);
    const mat4 view = mat4_look_at(eye, target, up);

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

        double seconds = (double)(SDL_GetPerformanceCounter() - start) / (double)freq;
        float t = (float)seconds;

        // Build the model matrix: spin around y, and a bit around x,
        // so we see the cube from a lively angle. Then chain the three
        // matrices into one (reads right-to-left: model, then view,
        // then projection — README section 5.11).
        mat4 model = mat4_mul(mat4_rotation_y(t * 0.8f),
            mat4_rotation_x(t * 0.5f));
        mat4 mvp = mat4_mul(projection, mat4_mul(view, model));

        // Run all 8 cube corners through the pipeline, storing the
        // resulting screen pixel of each. `visible[i]` says whether
        // corner i ended up in front of the camera.
        int  screen_x[8], screen_y[8];
        bool visible[8];
        for (int i = 0; i < 8; i++) {
            // Step 1 — one matrix multiply (the vertex carries w = 1).
            vec4 clip = mat4_mul_vec4(mvp, vec4_from_point(CUBE_VERTS[i]));

            // Behind the camera? w came out <= 0; dividing would give
            // garbage, so skip this corner (proper clipping is ch. 12).
            if (clip.w <= 0.0001f) { visible[i] = false; continue; }

            // Step 2 — the perspective divide. This is where distant
            // things actually get smaller: x and y are divided by the
            // depth that the projection matrix parked in w.
            float ndc_x = clip.x / clip.w;     // now in -1..+1
            float ndc_y = clip.y / clip.w;

            // Step 3 — viewport remap: NDC (-1..+1) to pixels. The y
            // term flips top-for-bottom because screen rows count down
            // (chapter 1), while NDC has +y up.
            screen_x[i] = (int)((ndc_x + 1.0f) * 0.5f * WIDTH);
            screen_y[i] = (int)((1.0f - (ndc_y + 1.0f) * 0.5f) * HEIGHT);
            visible[i] = true;
        }

        clear(rgb(16, 16, 24));

        // Draw the 12 edges, each connecting two of the projected
        // corners. v0/v1 are the corner indices; axis (0/1/2) picks
        // the edge's colour so x/y/z edges read as red/green/blue.
        for (int i = 0; i < 12; i++) {
            int v0 = CUBE_EDGES[i][0];
            int v1 = CUBE_EDGES[i][1];
            int axis = CUBE_EDGES[i][2];
            if (!visible[v0] || !visible[v1]) continue;
            draw_line(screen_x[v0], screen_y[v0],
                screen_x[v1], screen_y[v1], axis_color[axis]);
        }

        SDL_UpdateTexture(texture, NULL, framebuffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - last) / (double)freq;
        last = now;
        fps_accum += dt;
        fps_count += 1;
        if (fps_accum >= 1.0) {
            double fps = (double)fps_count / fps_accum;
            double ms = 1000.0 * fps_accum / (double)fps_count;
            snprintf(title, sizeof title,
                "Chapter 05 — 3D pipeline  |  %.1f FPS  (%.2f ms/frame)",
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

