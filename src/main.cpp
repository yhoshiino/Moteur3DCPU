// ============================================================
//  Chapter 12 (+) — Shaders and materials
// ------------------------------------------------------------
//  The rasterizer no longer decides the colour of a pixel. It
//  interpolates the per-pixel data (normal, UV, depth), packs it
//  in a FragmentInput, and asks the object's MATERIAL which
//  SHADER to call. The shader returns the colour.
//
//    rasterizer  -> FragmentInput -> shader(mat, scene) -> colour
//
//  Keys 1..5 switch the material of the cow at runtime.
//  Key 1 (Phong) must look exactly like chapter 12.
// ============================================================

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "font.h"

#include "vec.h"
#include "mat.h"
#include "obj.h"
#include "texture.h"
#include "clip.h"

#define WIDTH  800
#define HEIGHT 600

#ifndef WARP_ASSET_DIR
#define WARP_ASSET_DIR "assets"
#endif

static uint32_t framebuffer[WIDTH * HEIGHT];
static float    zbuffer[WIDTH * HEIGHT];

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)0xFFu << 24)
        | ((uint32_t)r << 16)
        | ((uint32_t)g << 8)
        | (uint32_t)b;
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

static inline float pow32(float x) {
    float x2 = x * x, x4 = x2 * x2, x8 = x4 * x4, x16 = x8 * x8;
    return x16 * x16;
}

static inline uint32_t color_phong(uint32_t texel, float diffuse_k, float spec_k) {
    float r = (float)((texel >> 16) & 0xFF) * diffuse_k + 255.0f * spec_k;
    float g = (float)((texel >> 8) & 0xFF) * diffuse_k + 255.0f * spec_k;
    float b = (float)(texel & 0xFF) * diffuse_k + 255.0f * spec_k;
    if (r > 255.0f) r = 255.0f;
    if (g > 255.0f) g = 255.0f;
    if (b > 255.0f) b = 255.0f;
    return ((uint32_t)0xFFu << 24)
        | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// ============================================================
//  Shader system
// ============================================================

// What the rasterizer gives the shader for ONE pixel. These values
// change from pixel to pixel ("varyings"): they are interpolated.
typedef struct {
    vec3  normal;   // interpolated and re-normalised, world space
    vec2  uv;       // perspective-correct texture coordinate
    float depth;    // NDC z of this pixel
} FragmentInput;

// What is the same for the WHOLE scene ("uniforms" of the scene).
typedef struct {
    vec3 light;      // unit vector pointing TOWARD the light
    vec3 half_vec;   // halfway between light and viewer (Blinn-Phong)
} Scene;

// A shader is a function: (pixel data, material, scene) -> colour.
// Returning a colour with alpha == 0 means "discard this pixel".
struct Material;
typedef uint32_t(*FragmentShader)(const FragmentInput* in,
    const struct Material* mat, const Scene* scene);

// A material = the parameters of ONE object + which shader uses them
// ("uniforms" of the object). Two materials can share one shader.
typedef struct Material {
    const char* name;
    FragmentShader shader;
    const Texture* tex;
    uint32_t       color;          // used by the solid-colour shader
    float          ambient;
    float          spec_strength;
    bool           bilinear;
} Material;

static inline uint32_t sample_texture(const Material* mat, vec2 uv) {
    return mat->bilinear
        ? texture_sample_bilinear(mat->tex, uv.x, uv.y)
        : texture_sample(mat->tex, uv.x, uv.y);
}

// ---- The shaders ---------------------------------------------

// Texture * (ambient + diffuse) + white specular. This is exactly
// the code that used to be inside draw_triangle_phong.
static uint32_t shader_phong(const FragmentInput* in, const Material* mat, const Scene* scene) {
    float ndotl = vec3_dot(in->normal, scene->light);
    if (ndotl < 0.0f) ndotl = 0.0f;
    float diffuse_k = mat->ambient + (1.0f - mat->ambient) * ndotl;

    float ndoth = vec3_dot(in->normal, scene->half_vec);
    if (ndoth < 0.0f) ndoth = 0.0f;
    float spec_k = pow32(ndoth) * mat->spec_strength;

    return color_phong(sample_texture(mat, in->uv), diffuse_k, spec_k);
}

// Same as Phong but without the specular highlight (matte).
static uint32_t shader_lambert(const FragmentInput* in, const Material* mat, const Scene* scene) {
    float ndotl = vec3_dot(in->normal, scene->light);
    if (ndotl < 0.0f) ndotl = 0.0f;
    float diffuse_k = mat->ambient + (1.0f - mat->ambient) * ndotl;

    return color_phong(sample_texture(mat, in->uv), diffuse_k, 0.0f);
}

// Texture only, no lighting at all.
static uint32_t shader_texture(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)scene;
    return sample_texture(mat, in->uv) | 0xFF000000u;
}

// One flat colour, ignores everything else.
static uint32_t shader_solid(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)in; (void)scene;
    return mat->color;
}

// Debug shader: shows the normal as a colour. N is in [-1, 1] on each
// axis, so (N * 0.5 + 0.5) brings it to [0, 1], then * 255.
static uint32_t shader_normals(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)mat; (void)scene;
    return rgb((uint8_t)((in->normal.x * 0.5f + 0.5f) * 255.0f),
        (uint8_t)((in->normal.y * 0.5f + 0.5f) * 255.0f),
        (uint8_t)((in->normal.z * 0.5f + 0.5f) * 255.0f));
}

// ============================================================
//  Rasterizer: same as chapter 12, except the end of the pixel loop
// ============================================================

static void draw_triangle_shaded(
    int x0, int y0, float z0, float iw0, vec2 uv0, vec3 n0,
    int x1, int y1, float z1, float iw1, vec2 uv1, vec3 n1,
    int x2, int y2, float z2, float iw2, vec2 uv2, vec3 n2,
    const Material* mat, const Scene* scene) {
    int area = edge(x0, y0, x1, y1, x2, y2);
    if (area == 0) return;
    if (area < 0) {
        int   tx = x1, ty = y1; float tz = z1, tiw = iw1; vec2 tuv = uv1; vec3 tn = n1;
        x1 = x2; y1 = y2; z1 = z2; iw1 = iw2; uv1 = uv2; n1 = n2;
        x2 = tx; y2 = ty; z2 = tz; iw2 = tiw; uv2 = tuv; n2 = tn;
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
    vec3 n0w = vec3_scale(n0, iw0);
    vec3 n1w = vec3_scale(n1, iw1);
    vec3 n2w = vec3_scale(n2, iw2);

    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            int e0 = edge(x1, y1, x2, y2, x, y);
            int e1 = edge(x2, y2, x0, y0, x, y);
            int e2 = edge(x0, y0, x1, y1, x, y);
            if ((e0 | e1 | e2) < 0) continue;

            float l0 = e0 * inv_area, l1 = e1 * inv_area, l2 = e2 * inv_area;

            float z = l0 * z0 + l1 * z1 + l2 * z2;
            int idx = y * WIDTH + x;
            if (z >= zbuffer[idx]) continue;

            float iw = l0 * iw0 + l1 * iw1 + l2 * iw2;
            float inv_iw = 1.0f / iw;
            float u = (l0 * u0w + l1 * u1w + l2 * u2w) * inv_iw;
            float v = (l0 * v0w + l1 * v1w + l2 * v2w) * inv_iw;

            vec3 N = vec3_make((l0 * n0w.x + l1 * n1w.x + l2 * n2w.x) * inv_iw,
                (l0 * n0w.y + l1 * n1w.y + l2 * n2w.y) * inv_iw,
                (l0 * n0w.z + l1 * n1w.z + l2 * n2w.z) * inv_iw);
            N = vec3_normalize(N);

            // The only new part: hand the pixel data to the shader.
            FragmentInput in;
            in.normal = N;
            in.uv = vec2_make(u, v);
            in.depth = z;

            uint32_t color = mat->shader(&in, mat, scene);
            if ((color >> 24) == 0) continue;     // shader discarded the pixel

            // The z-buffer stays the rasterizer's job, never the shader's.
            zbuffer[idx] = z;
            framebuffer[idx] = color;
        }
    }
}

static void project_and_draw(const clip_vertex* tri, const Material* mat, const Scene* scene) {
    int   screen_x[3], screen_y[3];
    float ndc_z[3], inv_w[3];
    for (int k = 0; k < 3; k++) {
        float inv = 1.0f / tri[k].clip.w;
        screen_x[k] = (int)((tri[k].clip.x * inv + 1.0f) * 0.5f * WIDTH);
        screen_y[k] = (int)((1.0f - (tri[k].clip.y * inv + 1.0f) * 0.5f) * HEIGHT);
        ndc_z[k] = tri[k].clip.z * inv;
        inv_w[k] = inv;
    }

    if (edge(screen_x[0], screen_y[0], screen_x[1], screen_y[1], screen_x[2], screen_y[2]) >= 0) return;  // cull

    draw_triangle_shaded(screen_x[0], screen_y[0], ndc_z[0], inv_w[0], tri[0].uv, tri[0].normal,
        screen_x[1], screen_y[1], ndc_z[1], inv_w[1], tri[1].uv, tri[1].normal,
        screen_x[2], screen_y[2], ndc_z[2], inv_w[2], tri[2].uv, tri[2].normal,
        mat, scene);
}

static void compute_face_normals(const Mesh* mesh, vec3* out) {
    for (int f = 0; f < mesh->face_count; f++) {
        vec3 v0 = mesh->verts[mesh->faces[f * 3 + 0]];
        vec3 v1 = mesh->verts[mesh->faces[f * 3 + 1]];
        vec3 v2 = mesh->verts[mesh->faces[f * 3 + 2]];
        out[f] = vec3_normalize(vec3_cross(vec3_sub(v1, v0), vec3_sub(v2, v0)));
    }
}

static void compute_vertex_normals(const Mesh* mesh, const vec3* face_normals, vec3* out) {
    for (int i = 0; i < mesh->vert_count; i++) out[i] = vec3_make(0, 0, 0);
    for (int f = 0; f < mesh->face_count; f++)
        for (int k = 0; k < 3; k++)
            out[mesh->faces[f * 3 + k]] = vec3_add(out[mesh->faces[f * 3 + k]], face_normals[f]);
    for (int i = 0; i < mesh->vert_count; i++) out[i] = vec3_normalize(out[i]);
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

    vec3* face_normals = (vec3*)malloc(mesh.face_count * sizeof(vec3));
    vec3* vertex_normals = (vec3*)malloc(mesh.vert_count * sizeof(vec3));
    compute_face_normals(&mesh, face_normals);
    compute_vertex_normals(&mesh, face_normals, vertex_normals);

    Font font;
    if (!font_load(WARP_ASSET_DIR "/fonts/Basic Light.ttf", 20.0f, &font)) return 1;
    char hud[64] = "Hello Spot";

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Chapter 12 — Shaders", WIDTH, HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* screen_tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    const float aspect = (float)WIDTH / (float)HEIGHT;
    const mat4 projection = mat4_perspective(radians(50.0f), aspect, 0.1f, 100.0f);
    const vec3 eye = vec3_make(0, 0, 3);
    const vec3 target = vec3_make(0, 0, 0);
    const vec3 up = vec3_make(0, 1, 0);
    const mat4 view = mat4_look_at(eye, target, up);

    const vec3 light = vec3_normalize(vec3_make(0.4f, 0.8f, 0.45f));
    const vec3 half_vec = vec3_normalize(vec3_add(light, vec3_make(0, 0, 1)));

    // Scene-wide data, shared by every material.
    const Scene scene = { light, half_vec };

    // The materials. Field order:
    //   name, shader, texture, solid colour, ambient, spec strength, bilinear
    // Material 1 uses the same values as the old #defines (0.18 / 0.55 / bilinear).
    const Material materials[] = {
        { "Phong",            shader_phong,   &tex, 0,                 0.18f, 0.55f, true  },
        { "Lambert (matte)",  shader_lambert, &tex, 0,                 0.18f, 0.0f,  true  },
        { "Texture only",     shader_texture, &tex, 0,                 0.0f,  0.0f,  true  },
        { "Solid colour",     shader_solid,   NULL, rgb(220, 120, 60), 0.0f,  0.0f,  false },
        { "Normals",          shader_normals, NULL, 0,                 0.0f,  0.0f,  false },
    };
    const int material_count = (int)(sizeof materials / sizeof materials[0]);
    int current = 0;

    vec4* clip_v = (vec4*)malloc(mesh.vert_count * sizeof(vec4));
    vec3* normal_world = (vec3*)malloc(mesh.vert_count * sizeof(vec3));

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
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) running = false;
                // Keys '1'..'5' select a material.
                int pick = (int)e.key.key - (int)SDLK_1;
                if (pick >= 0 && pick < material_count) current = pick;
            }
        }

        double seconds = (double)(SDL_GetPerformanceCounter() - start) / (double)freq;
        float t = (float)seconds;

        mat4 model = mat4_rotation_y(t * 0.7f);
        mat4 mvp = mat4_mul(projection, mat4_mul(view, model));

        for (int i = 0; i < mesh.vert_count; i++) {
            clip_v[i] = mat4_mul_vec4(mvp, vec4_from_point(mesh.verts[i]));
            normal_world[i] = mat4_mul_dir(model, vertex_normals[i]);
        }

        clear(rgb(16, 16, 24));
        clear_zbuffer();

        for (int fidx = 0; fidx < mesh.face_count; fidx++) {
            int a = mesh.faces[fidx * 3 + 0];
            int b = mesh.faces[fidx * 3 + 1];
            int c = mesh.faces[fidx * 3 + 2];
            int ua = mesh.face_uvs[fidx * 3 + 0];
            int ub = mesh.face_uvs[fidx * 3 + 1];
            int uc = mesh.face_uvs[fidx * 3 + 2];

            clip_vertex v0, v1, v2;
            v0.clip = clip_v[a]; v0.normal = normal_world[a]; v0.uv = (ua >= 0) ? mesh.uvs[ua] : vec2_make(0, 0);
            v1.clip = clip_v[b]; v1.normal = normal_world[b]; v1.uv = (ub >= 0) ? mesh.uvs[ub] : vec2_make(0, 0);
            v2.clip = clip_v[c]; v2.normal = normal_world[c]; v2.uv = (uc >= 0) ? mesh.uvs[uc] : vec2_make(0, 0);

            clip_vertex out[6];
            int n_out;
            clip_triangle_near(v0, v1, v2, out, &n_out);
            for (int ot = 0; ot < n_out; ot++)
                project_and_draw(&out[ot * 3], &materials[current], &scene);
        }

        font_draw_text(framebuffer, WIDTH, HEIGHT, &font, 10, 24, hud, rgb(255, 255, 255));
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

            snprintf(hud, sizeof hud, "%.1f FPS", fps);

            snprintf(title, sizeof title,
                "Chapter 12 — Shaders  |  [%d] %s  |  %.1f FPS  (%.2f ms)",
                current + 1, materials[current].name, fps, ms);
            SDL_SetWindowTitle(window, title);
            fps_accum = 0.0;
            fps_count = 0;
        }
    }

    free(clip_v); free(normal_world);
    free(face_normals); free(vertex_normals);
    mesh_free(&mesh);
    font_free(&font);
    texture_free(&tex);
    SDL_DestroyTexture(screen_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}