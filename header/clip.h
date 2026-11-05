#pragma once
#include "vec.h"


struct clip_vertex {
    vec4 clip;
    vec2 uv;
    vec3 normal;
};

inline float lerpf(float a, float b, float t) { return a + t * (b - a); }

inline clip_vertex clip__lerp(const clip_vertex& a, const clip_vertex& b, float t) {
    return {
        { lerpf(a.clip.x, b.clip.x, t), lerpf(a.clip.y, b.clip.y, t),
          lerpf(a.clip.z, b.clip.z, t), lerpf(a.clip.w, b.clip.w, t) },
        { lerpf(a.uv.x, b.uv.x, t), lerpf(a.uv.y, b.uv.y, t) },
        { lerpf(a.normal.x, b.normal.x, t), lerpf(a.normal.y, b.normal.y, t),
          lerpf(a.normal.z, b.normal.z, t) }
    };
}

static inline float clip__near_dist(clip_vertex v) {
    return v.clip.z + v.clip.w;
}

static void clip_triangle_near(clip_vertex v0, clip_vertex v1, clip_vertex v2, clip_vertex* out, int* out_n_tris) {
    clip_vertex in[3] = { v0, v1, v2 };
    clip_vertex poly[4];
    int n = 0;

    for (int e = 0; e < 3; e++) {
        clip_vertex a = in[e];
        clip_vertex b = in[(e + 1) % 3];
        float da = clip__near_dist(a);
        float db = clip__near_dist(b);
        bool a_in = da >= 0.0f;
        bool b_in = db >= 0.0f;

        if (a_in && b_in) {
            poly[n++] = b;
        }
        else if (a_in && !b_in) {
            float t = da / (da - db);
            poly[n++] = clip__lerp(a, b, t);
        }
        else if (!a_in && b_in) {
            float t = da / (da - db);
            poly[n++] = clip__lerp(a, b, t);
            poly[n++] = b;
        }
    }

    if (n == 3) {
        out[0] = poly[0]; out[1] = poly[1]; out[2] = poly[2];
        *out_n_tris = 1;
    }
    else if (n == 4) {
        out[0] = poly[0]; out[1] = poly[1]; out[2] = poly[2];
        out[3] = poly[0]; out[4] = poly[2]; out[5] = poly[3];
        *out_n_tris = 2;
    }
    else {
        *out_n_tris = 0;
    }
}