// ============================================================
//  Chapter 10 — Diffuse lighting: letting the sun shine on Spot
// ------------------------------------------------------------
//  Chapter 9 textured Spot, but every face was equally bright —
//  no sense of a light or which way the surface faced. Now we
//  add a directional light and shade each triangle by how
//  squarely it faces that light (Lambert's cosine law):
//
//      brightness = ambient + (1 - ambient) * max(0, dot(N, L))
//      final      = texel * brightness
//
//  N is the triangle's surface normal in WORLD space (the model-
//  space normal rotated by the model matrix each frame). L is a
//  fixed unit vector pointing TOWARD the light. The dot product is
//  the cosine of the angle between them — Lambert's law (1760) as
//  one vector operation.
//
//  Shading is FLAT: one brightness per triangle. Smooth (Phong)
//  shading is chapter 11. Texture (ch 9), Z-buffer (ch 7) and
//  backface cull (ch 8) all stay on.
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
#include "texture.h"

#define WIDTH  800
#define HEIGHT 600

#ifndef WARP_ASSET_DIR
#define WARP_ASSET_DIR "assets"
#endif

// Ambient floor: how lit a surface is even when it faces away from
// the light. 0.0 = pitch black on the dark side; 1.0 = no shading
// at all. 0.18 keeps the texture readable in shadow.
#define AMBIENT 0.18f

static uint32_t framebuffer[WIDTH * HEIGHT];
static float    zbuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)0xFFu << 24) | ((uint32_t)r << 16)
        | ((uint32_t)g << 8) | (uint32_t)b;
}

static void clear(uint32_t color) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) framebuffer[i] = color;
}

static void clear_zbuffer(void) {
    for (int i = 0; i < WIDTH * HEIGHT; i++) zbuffer[i] = 1e30f;
}

static inline int edge(int ax, int ay, int bx, int by, int px, int py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

static inline int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Scale an ARGB texel by a brightness in [0, 1], per channel.
static inline uint32_t color_scale(uint32_t c, float k) {
    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    uint32_t r = (uint32_t)((float)((c >> 16) & 0xFF) * k);
    uint32_t g = (uint32_t)((float)((c >> 8) & 0xFF) * k);
    uint32_t b = (uint32_t)((float)(c & 0xFF) * k);
    return ((uint32_t)0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// Per-triangle surface normal in MODEL space. Computed once at load:
// the cross product of two edges is perpendicular to the triangle,
// and we normalise it to unit length. The winding (chapter 8) makes
// this point OUT of the front face.
static void compute_face_normals(const Mesh* mesh, vec3* out) {
    for (int f = 0; f < mesh->face_count; f++) {
        vec3 v0 = mesh->verts[mesh->faces[f * 3 + 0]];
        vec3 v1 = mesh->verts[mesh->faces[f * 3 + 1]];
        vec3 v2 = mesh->verts[mesh->faces[f * 3 + 2]];
        vec3 e1 = vec3_sub(v1, v0);
        vec3 e2 = vec3_sub(v2, v0);
        out[f] = vec3_normalize(vec3_cross(e1, e2));
    }
}

// Textured + lit triangle: perspective-correct UV (chapter 9) plus a
// single flat brightness multiplying every sampled texel.
static void draw_triangle_lit(int x0, int y0, float z0, float iw0, vec2 uv0,
    int x1, int y1, float z1, float iw1, vec2 uv1,
    int x2, int y2, float z2, float iw2, vec2 uv2,
    const Texture* tex, float brightness) {
    int area = edge(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;

    if (area < 0) {
        int tx = x1, ty = y1; float tz = z1, tiw = iw1; vec2 tuv = uv1;
        x1 = x2; y1 = y2; z1 = z2; iw1 = iw2; uv1 = uv2;
        x2 = tx; y2 = ty; z2 = tz; iw2 = tiw; uv2 = tuv;
        area = -area;
    }

    int minx = clampi(x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), 0, WIDTH - 1);
    int miny = clampi(y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), 0, HEIGHT - 1);
    int maxx = clampi(x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2), 0, WIDTH - 1);
    int maxy = clampi(y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2), 0, HEIGHT - 1);

    float inv_area = 1.0f / (float)area;
    float u0w = uv0.x * iw0, v0w = uv0.y * iw0;
    float u1w = uv1.x * iw1, v1w = uv1.y * iw1;
    float u2w = uv2.x * iw2, v2w = uv2.y * iw2;

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            int e0 = edge(x1, y1, x2, y2, x, y);
            int e1 = edge(x2, y2, x0, y0, x, y);
            int e2 = edge(x0, y0, x1, y1, x, y);
            if ((e0 | e1 | e2) < 0) continue;

            float l0 = e0 * inv_area;
            float l1 = e1 * inv_area;
            float l2 = e2 * inv_area;

            float z = l0 * z0 + l1 * z1 + l2 * z2;
            int idx = y * WIDTH + x;
            if (z >= zbuffer[idx]) continue;

            float uw = l0 * u0w + l1 * u1w + l2 * u2w;
            float vw = l0 * v0w + l1 * v1w + l2 * v2w;
            float iw = l0 * iw0 + l1 * iw1 + l2 * iw2;
            uint32_t texel = texture_sample(tex, uw / iw, vw / iw);

            zbuffer[idx] = z;
            framebuffer[idx] = color_scale(texel, brightness);
        }
    }
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    Mesh mesh;
    if (!mesh_load(WARP_ASSET_DIR "/spot.obj", &mesh)) return 1;
    printf("loaded spot.obj: %d vertices, %d uvs, %d faces\n",
        mesh.vert_count, mesh.uv_count, mesh.face_count);

    Texture tex;
    if (!texture_load(WARP_ASSET_DIR "/spot_texture.png", &tex)) return 1;
    printf("loaded spot_texture.png: %dx%d texels\n", tex.width, tex.height);

    // One model-space normal per triangle, computed once.
    vec3* face_normals = (vec3*)malloc(mesh.face_count * sizeof(vec3));
    compute_face_normals(&mesh, face_normals);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Chapter 10 — Diffuse lighting", WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* screen_tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    const float aspect = (float)WIDTH / (float)HEIGHT;
    const mat4 projection = mat4_perspective(radians(50.0f), aspect, 0.1f, 100.0f);
    const vec3 eye = vec3_make(0, 0, 3);
    const vec3 target = vec3_make(0, 0, 0);
    const vec3 up = vec3_make(0, 1, 0);
    const mat4 view = mat4_look_at(eye, target, up);

    // Directional light, fixed in WORLD space. L points FROM the
    // surface TOWARD the light — here, up and a bit to the front-right,
    // like a sun over your shoulder.
    const vec3 light = vec3_normalize(vec3_make(0.4f, 0.8f, 0.45f));

    int* screen_x = (int*)malloc(mesh.vert_count * sizeof(int));
    int* screen_y = (int*)malloc(mesh.vert_count * sizeof(int));
    float* ndc_z = (float*)malloc(mesh.vert_count * sizeof(float));
    float* inv_w = (float*)malloc(mesh.vert_count * sizeof(float));
    bool* visible = (bool*)malloc(mesh.vert_count * sizeof(bool));

    const Uint64 freq = SDL_GetPerformanceFrequency();
    const Uint64 start = SDL_GetPerformanceCounter();
    Uint64 last = start;
    double fps_accum = 0.0;
    int    fps_count = 0;
    char   title[180];

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

        for (int i = 0; i < mesh.vert_count; i++) {
            vec4 clip = mat4_mul_vec4(mvp, vec4_from_point(mesh.verts[i]));
            if (clip.w <= 0.0001f) { visible[i] = false; continue; }
            float inv = 1.0f / clip.w;
            screen_x[i] = (int)((clip.x * inv + 1.0f) * 0.5f * WIDTH);
            screen_y[i] = (int)((1.0f - (clip.y * inv + 1.0f) * 0.5f) * HEIGHT);
            ndc_z[i] = clip.z * inv;
            inv_w[i] = inv;
            visible[i] = true;
        }

        clear(rgb(16, 16, 24));
        clear_zbuffer();

        for (int fidx = 0; fidx < mesh.face_count; fidx++) {
            int a = mesh.faces[fidx * 3 + 0];
            int b = mesh.faces[fidx * 3 + 1];
            int c = mesh.faces[fidx * 3 + 2];
            if (!visible[a] || !visible[b] || !visible[c]) continue;

            if (edge(screen_x[a], screen_y[a], screen_x[b], screen_y[b], screen_x[c], screen_y[c]) >= 0) continue;  // cull

            // Lambert shading for this face. Rotate the normal into
            // world space, then brightness = ambient + diffuse, where
            // diffuse = max(0, N·L). The model matrix here is a pure
            // rotation, so the normal stays unit length.
            vec3 n = mat4_mul_dir(model, face_normals[fidx]);
            float ndotl = vec3_dot(n, light);
            if (ndotl < 0.0f) ndotl = 0.0f;
            float brightness = AMBIENT + (1.0f - AMBIENT) * ndotl;

            int ua = mesh.face_uvs[fidx * 3 + 0];
            int ub = mesh.face_uvs[fidx * 3 + 1];
            int uc = mesh.face_uvs[fidx * 3 + 2];
            vec2 uva = (ua >= 0) ? mesh.uvs[ua] : vec2_make(0, 0);
            vec2 uvb = (ub >= 0) ? mesh.uvs[ub] : vec2_make(0, 0);
            vec2 uvc = (uc >= 0) ? mesh.uvs[uc] : vec2_make(0, 0);

            draw_triangle_lit(screen_x[a], screen_y[a], ndc_z[a], inv_w[a], uva,
                screen_x[b], screen_y[b], ndc_z[b], inv_w[b], uvb,
                screen_x[c], screen_y[c], ndc_z[c], inv_w[c], uvc,
                &tex, brightness);
        }

        SDL_UpdateTexture(screen_tex, NULL, framebuffer, WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, screen_tex, NULL, NULL);
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
                "Chapter 10 — Diffuse lighting  |  flat Lambert  |  %.1f FPS  (%.2f ms)",
                fps, ms);
            SDL_SetWindowTitle(window, title);
            fps_accum = 0.0;
            fps_count = 0;
        }
    }

    free(screen_x); free(screen_y); free(ndc_z); free(inv_w); free(visible);
    free(face_normals);
    mesh_free(&mesh);
    texture_free(&tex);
    SDL_DestroyTexture(screen_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}