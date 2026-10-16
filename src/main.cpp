// ============================================================
//  Chapter 06 — Loading 3D meshes: wireframe Spot
// ------------------------------------------------------------
//  Loads spot.obj and draws it as a spinning wireframe, using
//  the exact chapter-5 pipeline (MVP, perspective divide,
//  viewport) on every loaded vertex. Edges are shaded by depth
//  so the busy wireframe stays readable until chapter 7 gives
//  us a real Z-buffer.
//
//  WARP_ASSET_DIR is provided by the build (see CMakeLists.txt)
//  so the .exe finds spot.obj from any working directory.
// ============================================================

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "vec.h"
#include "mat.h"
#include "obj.h"

#define WIDTH  800
#define HEIGHT 600

#ifndef WARP_ASSET_DIR
#define WARP_ASSET_DIR "assets"
#endif

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
    int step_x = (x0 < x1) ? 1 : -1;
    int step_y = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += step_x; }
        if (e2 <= dx) { err += dx; y0 += step_y; }
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    // ---- Load the mesh ----
    Mesh mesh;
    if (!mesh_load(WARP_ASSET_DIR "/spot.obj", &mesh)) return 1;
    printf("loaded spot.obj: %d vertices, %d faces\n",
        mesh.vert_count, mesh.face_count);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Chapter 06 — Loading meshes", WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    const float aspect = (float)WIDTH / (float)HEIGHT;

    // 50-degree vertical field of view (radians() converts to radians).
    const mat4 projection = mat4_perspective(radians(50.0f), aspect,
        0.1f, 100.0f);

    // Camera 3 units back, looking at the origin where Spot sits
    // (she's ~1 unit tall), with +y as world-up.
    const vec3 eye = vec3_make(0, 0, 3);
    const vec3 target = vec3_make(0, 0, 0);
    const vec3 up = vec3_make(0, 1, 0);
    const mat4 view = mat4_look_at(eye, target, up);

    // Per-vertex scratch buffers (projected screen coords + depth).
    int* screen_x = (int*)malloc(mesh.vert_count * sizeof(int));
    int* screen_y = (int*)malloc(mesh.vert_count * sizeof(int));
    float* cam_depth = (float*)malloc(mesh.vert_count * sizeof(float));
    bool* visible = (bool*)malloc(mesh.vert_count * sizeof(bool));

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

        mat4 model = mat4_rotation_y(t * 0.7f);
        mat4 mvp = mat4_mul(projection, mat4_mul(view, model));

        // Project every vertex once. We store the CAMERA-SPACE depth
        // (clip.w, which equals the distance in front of the camera)
        // rather than the NDC z. NDC z is hyperbolic and, for a small
        // object far from the near plane, barely changes across the
        // whole model — so a depth cue based on it would be invisible.
        // Camera-space depth varies linearly, giving a usable range.
        float dmin = 1e30f, dmax = -1e30f;
        for (int i = 0; i < mesh.vert_count; i++) {
            vec4 clip = mat4_mul_vec4(mvp, vec4_from_point(mesh.verts[i]));
            if (clip.w <= 0.0001f) { visible[i] = false; continue; }
            float ndc_x = clip.x / clip.w;
            float ndc_y = clip.y / clip.w;
            screen_x[i] = (int)((ndc_x + 1.0f) * 0.5f * WIDTH);
            screen_y[i] = (int)(((1.0f - ndc_y) * 0.5f) * HEIGHT);
            cam_depth[i] = clip.w;                       // camera-space depth (bigger = farther)
            visible[i] = true;
            if (clip.w < dmin) dmin = clip.w;
            if (clip.w > dmax) dmax = clip.w;
        }
        float drange = (dmax > dmin) ? (dmax - dmin) : 1.0f;

        clear(rgb(16, 16, 24));

        // Draw each triangle's three edges, shaded by depth. We map
        // the model's own near..far depth span onto a bright..dim
        // grey range, so the cue is clearly visible regardless of how
        // far the object sits from the camera.
        for (int fidx = 0; fidx < mesh.face_count; fidx++) {
            int a = mesh.faces[fidx * 3 + 0];
            int b = mesh.faces[fidx * 3 + 1];
            int c = mesh.faces[fidx * 3 + 2];
            if (!visible[a] || !visible[b] || !visible[c]) continue;

            float avg_d = (cam_depth[a] + cam_depth[b] + cam_depth[c]) / 3.0f;
            float far01 = (avg_d - dmin) / drange;            // 0 near, 1 far
            int level = 240 - (int)(far01 * 200.0f);          // 240 near .. 40 far
            if (level < 0) level = 0; if (level > 255) level = 255;
            uint32_t color = rgb((uint8_t)level, (uint8_t)level, (uint8_t)level);

            draw_line(screen_x[a], screen_y[a], screen_x[b], screen_y[b], color);
            draw_line(screen_x[b], screen_y[b], screen_x[c], screen_y[c], color);
            draw_line(screen_x[c], screen_y[c], screen_x[a], screen_y[a], color);
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
                "Chapter 06 — Loading meshes  |  %d tris  |  %.1f FPS  (%.2f ms)",
                mesh.face_count, fps, ms);
            SDL_SetWindowTitle(window, title);
            fps_accum = 0.0;
            fps_count = 0;
        }
    }

    free(screen_x); free(screen_y); free(cam_depth); free(visible);
    mesh_free(&mesh);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}