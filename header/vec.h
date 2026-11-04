#pragma once

using vec2 = struct { float x; float y; };
using vec3 = struct { float x; float y; float z; };
using vec4 = struct { float x; float y; float z; float w; };

static inline vec2 vec2_make(float x, float y) {
    vec2 v = { x, y };
    return v;
}

static inline vec3 vec3_make(float x, float y, float z) {
    vec3 v = { x, y, z };
    return v;
}

static inline vec3 vec3_add(vec3 a, vec3 b) {
    return vec3_make(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline vec3 vec3_sub(vec3 a, vec3 b) {
    return vec3_make(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline vec3 vec3_scale(vec3 a, float s) {
    return vec3_make(a.x * s, a.y * s, a.z * s);
}

static inline float vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3 vec3_cross(vec3 a, vec3 b) {
    return vec3_make(a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x);
}

static inline float vec3_length(vec3 a) {
    return sqrtf(vec3_dot(a, a));
}

static inline vec3 vec3_normalize(vec3 a) {
    float len = vec3_length(a);
    if (len > 0.0f) return vec3_scale(a, 1.0f / len);
    return a;
}

static inline vec4 vec4_from_point(vec3 p) {
    vec4 v = { p.x, p.y, p.z, 1.0f };
    return v;
}