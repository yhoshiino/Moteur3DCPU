#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "vec.h"
#include "texture.h"
#include "color.h"

typedef struct {
    vec3  normal;
    vec2  uv;
    float depth;
} FragmentInput;

typedef struct {
    vec3 light;
    vec3 half_vec;
} Scene;

struct Material;
typedef uint32_t(*FragmentShader)(const FragmentInput* in,
    const struct Material* mat, const Scene* scene);

typedef struct Material {
    const char* name;
    FragmentShader shader;
    const Texture* tex;
    uint32_t       color;
    float          ambient;
    float          spec_strength;
    bool           bilinear;
} Material;

static inline uint32_t sample_texture(const Material* mat, vec2 uv) {
    return mat->bilinear
        ? texture_sample_bilinear(mat->tex, uv.x, uv.y)
        : texture_sample(mat->tex, uv.x, uv.y);
}

static inline uint32_t shader_phong(const FragmentInput* in, const Material* mat, const Scene* scene) {
    float ndotl = vec3_dot(in->normal, scene->light);
    if (ndotl < 0.0f) ndotl = 0.0f;
    float diffuse_k = mat->ambient + (1.0f - mat->ambient) * ndotl;

    float ndoth = vec3_dot(in->normal, scene->half_vec);
    if (ndoth < 0.0f) ndoth = 0.0f;
    float spec_k = pow32(ndoth) * mat->spec_strength;

    return color_phong(sample_texture(mat, in->uv), diffuse_k, spec_k);
}

static inline uint32_t shader_lambert(const FragmentInput* in, const Material* mat, const Scene* scene) {
    float ndotl = vec3_dot(in->normal, scene->light);
    if (ndotl < 0.0f) ndotl = 0.0f;
    float diffuse_k = mat->ambient + (1.0f - mat->ambient) * ndotl;

    return color_phong(sample_texture(mat, in->uv), diffuse_k, 0.0f);
}

static inline uint32_t shader_texture(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)scene;
    return sample_texture(mat, in->uv) | 0xFF000000u;
}

static inline uint32_t shader_solid(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)in; (void)scene;
    return mat->color;
}

static inline uint32_t shader_normals(const FragmentInput* in, const Material* mat, const Scene* scene) {
    (void)mat; (void)scene;
    return rgb((uint8_t)((in->normal.x * 0.5f + 0.5f) * 255.0f),
        (uint8_t)((in->normal.y * 0.5f + 0.5f) * 255.0f),
        (uint8_t)((in->normal.z * 0.5f + 0.5f) * 255.0f));
}