// ============================================================
//  material.h — shaders and materials
// ------------------------------------------------------------
//  The rasterizer interpolates the per-pixel data (normal, UV,
//  depth), packs it in a FragmentInput, and asks the object's
//  MATERIAL which SHADER to call. The shader returns the colour.
//
//      rasterizer -> FragmentInput -> shader(mat, scene) -> colour
//
//  Rules for a shader:
//    - it only DECIDES a colour; it never touches the z-buffer
//      or the framebuffer (that is the rasterizer's job);
//    - returning a colour with alpha == 0 means "discard this
//      pixel" (the rasterizer skips it).
//
//  To add a shader: write a function with the FragmentShader
//  signature below, then add a line in the materials[] table.
// ============================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "vec.h"
#include "texture.h"
#include "color.h"

// What the rasterizer gives the shader for ONE pixel. These values
// change from pixel to pixel ("varyings"): they are interpolated.
typedef struct {
    vec3  normal;   // interpolated and re-normalised, world space
    vec2  uv;       // perspective-correct texture coordinate
    float depth;    // NDC z of this pixel
} FragmentInput;

// What is the same for the WHOLE scene.
typedef struct {
    vec3 light;      // unit vector pointing TOWARD the light
    vec3 half_vec;   // halfway between light and viewer (Blinn-Phong)
} Scene;

// A shader is a function: (pixel data, material, scene) -> colour.
struct Material;
typedef uint32_t(*FragmentShader)(const FragmentInput* in,
    const struct Material* mat, const Scene* scene);

// A material = the parameters of ONE object + which shader uses them.
// Two materials can share the same shader with different parameters.
typedef struct Material {
    const char* name;
    FragmentShader shader;
    const Texture* tex;            // may be NULL if the shader has no texture
    uint32_t       color;          // used by the solid-colour shader
    float          ambient;
    float          spec_strength;
    bool           bilinear;
} Material;

// Read the material's texture, bilinear or nearest depending on it.
static inline uint32_t sample_texture(const Material* mat, vec2 uv) {
    return mat->bilinear
        ? texture_sample_bilinear(mat->tex, uv.x, uv.y)
        : texture_sample(mat->tex, uv.x, uv.y);
}

// ---- The shaders ---------------------------------------------

// Texture * (ambient + diffuse) + white specular (chapter 11).
static inline uint32_t shader_phong(const FragmentInput* in, const Material* mat, const Scene* scene) {
    float ndotl = vec3_dot(in->normal, scene->light);
    if (ndotl < 0.0f) ndotl = 0.0f;
    float diffuse_k = mat->ambient + (1.0f - mat->ambient) * ndotl;

    float ndoth = vec3_dot(in->normal, scene->half_vec);
    if (ndoth < 0.0f) ndoth = 0.0f;
    float spec_k = pow32(ndoth) * mat->spec_strength;

    return color_phong(sample_texture(mat, in->uv), diffuse_k, spec_k);
}

// Same as Phong but without the specular highlight (matte).
static inline uint32_t shader_lambert(const FragmentInput* in, const Material* mat, const Scene* scene) {
    float ndotl = vec3_dot(in->normal, scene->light);
    if (ndotl < 0.0f) ndotl = 0.0f;
    float diffuse_k = mat->ambient + (1.0f - mat->ambient) * ndotl;

    return color_phong(sample_texture(mat, in->uv), diffuse_k, 0.0f);
}

// Texture only, no lighting at all.
static inline uint32_t shader_texture(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)scene;
    return sample_texture(mat, in->uv) | 0xFF000000u;
}

// One flat colour, ignores everything else.
static inline uint32_t shader_solid(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)in; (void)scene;
    return mat->color;
}

// Debug shader: shows the normal as a colour. N is in [-1, 1] on each
// axis, so (N * 0.5 + 0.5) brings it to [0, 1], then * 255.
static inline uint32_t shader_normals(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)mat; (void)scene;
    return rgb((uint8_t)((in->normal.x * 0.5f + 0.5f) * 255.0f),
        (uint8_t)((in->normal.y * 0.5f + 0.5f) * 255.0f),
        (uint8_t)((in->normal.z * 0.5f + 0.5f) * 255.0f));
}