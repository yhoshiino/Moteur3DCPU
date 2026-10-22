// ============================================================
//  clip.h — near-plane triangle clipping (Sutherland-Hodgman)
// ------------------------------------------------------------
//  Through chapter 11 we simply skipped any triangle with a vertex
//  behind the camera (clip.w <= 0). That misses the case that
//  matters: a triangle STRADDLING the near plane — some corners in
//  front, some behind. Skipping it leaves a hole; keeping it
//  produces garbage, because dividing by a w near 0 or negative
//  blows up.
//
//  The fix is to CLIP the triangle against the near plane *before*
//  the perspective divide, in CLIP SPACE. A corner behind the plane
//  is replaced by the point where its edge crosses the plane, with
//  every per-vertex attribute (clip position, UV, world normal)
//  linearly interpolated along that edge.
//
//  Inside test in clip space, near plane (NDC z = -1):
//      clip.z >= -clip.w   <=>   clip.z + clip.w >= 0
//  (works before the divide, which hasn't happened yet)
//
//  Clipping ONE triangle against ONE plane yields:
//      3 corners in -> 1 triangle (unchanged)
//      2 corners in -> a quad -> 2 triangles
//      1 corner  in -> 1 (smaller) triangle
//      0 corners in -> nothing
// ============================================================
#pragma once
#include "vec.h"
#include <stdbool.h>

// A vertex carrying every attribute we interpolate while clipping and
// that the rasterizer needs afterwards.
typedef struct {
    vec4 clip;     // clip-space position (post-MVP, pre-divide)
    vec2 uv;       // texture coordinate
    vec3 normal;   // world-space normal (already rotated)
} clip_vertex;

// Linearly interpolate every attribute: r = a*(1-t) + b*t.
static inline clip_vertex clip__lerp(clip_vertex a, clip_vertex b, float t) {
    clip_vertex r;
    r.clip.x = a.clip.x + t * (b.clip.x - a.clip.x);
    r.clip.y = a.clip.y + t * (b.clip.y - a.clip.y);
    r.clip.z = a.clip.z + t * (b.clip.z - a.clip.z);
    r.clip.w = a.clip.w + t * (b.clip.w - a.clip.w);
    r.uv.x = a.uv.x + t * (b.uv.x - a.uv.x);
    r.uv.y = a.uv.y + t * (b.uv.y - a.uv.y);
    r.normal.x = a.normal.x + t * (b.normal.x - a.normal.x);
    r.normal.y = a.normal.y + t * (b.normal.y - a.normal.y);
    r.normal.z = a.normal.z + t * (b.normal.z - a.normal.z);
    return r;
}

// Signed distance from the near plane, in clip-space units.
// Positive = in front of the near plane (kept).
static inline float clip__near_dist(clip_vertex v) {
    return v.clip.z + v.clip.w;
}

// Sutherland-Hodgman clip of one triangle against the near plane.
// Writes 0, 3, or 6 vertices to out (= 0, 1, or 2 triangles) and the
// triangle count to *out_n_tris. out must hold at least 6 entries.
static void clip_triangle_near(clip_vertex v0, clip_vertex v1, clip_vertex v2,
    clip_vertex* out, int* out_n_tris) {
    clip_vertex in[3] = { v0, v1, v2 };
    clip_vertex poly[4];          // 3 in -> at most 4 out for one plane
    int n = 0;

    // Walk the edges v0->v1, v1->v2, v2->v0. For each, append the
    // inside endpoint and/or the crossing point to the output polygon.
    for (int e = 0; e < 3; e++) {
        clip_vertex a = in[e];
        clip_vertex b = in[(e + 1) % 3];
        float da = clip__near_dist(a);
        float db = clip__near_dist(b);
        bool a_in = da >= 0.0f;
        bool b_in = db >= 0.0f;

        if (a_in && b_in) {
            poly[n++] = b;                       // both in: emit b
        }
        else if (a_in && !b_in) {
            float t = da / (da - db);            // leaving: emit crossing
            poly[n++] = clip__lerp(a, b, t);
        }
        else if (!a_in && b_in) {
            float t = da / (da - db);            // entering: crossing then b
            poly[n++] = clip__lerp(a, b, t);
            poly[n++] = b;
        }
        // both out: emit nothing
    }

    if (n == 3) {
        out[0] = poly[0]; out[1] = poly[1]; out[2] = poly[2];
        *out_n_tris = 1;
    }
    else if (n == 4) {
        // Fan-triangulate the quad: (0,1,2) and (0,2,3).
        out[0] = poly[0]; out[1] = poly[1]; out[2] = poly[2];
        out[3] = poly[0]; out[4] = poly[2]; out[5] = poly[3];
        *out_n_tris = 2;
    }
    else {
        *out_n_tris = 0;
    }
}