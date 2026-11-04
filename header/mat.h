#pragma once
#include "vec.h"
#include <math.h>

using mat4 = struct { float m[4][4]; };

static inline mat4 mat4_identity(void) {
    mat4 r = { {{0}} };
    for (int i = 0; i <= 3; i++) {
        r.m[i][i] = 1;
    }
    return r;
}

static inline mat4 mat4_mul(mat4 a, mat4 b) {
    mat4 r = { {{0}} };
    for (int i = 0; i <= 3; i++)
        for (int j = 0; j <= 3; j++) {
            for (int k = 0; k <= 3; k++) {
                r.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    return r;
}


static inline vec4 mat4_mul_vec4(mat4 a, vec4 v) {
    vec4 r;
    r.x = a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z + a.m[0][3] * v.w;
    r.y = a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z + a.m[1][3] * v.w;
    r.z = a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z + a.m[2][3] * v.w;
    r.w = a.m[3][0] * v.x + a.m[3][1] * v.y + a.m[3][2] * v.z + a.m[3][3] * v.w;
    return r;
}

static inline vec3 mat4_mul_dir(mat4 a, vec3 d) {
    return vec3_make(
        a.m[0][0] * d.x + a.m[0][1] * d.y + a.m[0][2] * d.z,
        a.m[1][0] * d.x + a.m[1][1] * d.y + a.m[1][2] * d.z,
        a.m[2][0] * d.x + a.m[2][1] * d.y + a.m[2][2] * d.z);
}

static inline mat4 mat4_translation(float tx, float ty, float tz) {
    mat4 r = mat4_identity();
    r.m[0][3] = tx; r.m[1][3] = ty; r.m[2][3] = tz;
    return r;
}

static inline mat4 mat4_scale(float s) {
    mat4 r = mat4_identity();
    r.m[0][0] = s; r.m[1][1] = s; r.m[2][2] = s;
    return r;
}

static inline mat4 mat4_rotation_x(float a) {
    mat4 r = mat4_identity();
    float c = cosf(a), s = sinf(a);
    r.m[1][1] = c; r.m[1][2] = -s;
    r.m[2][1] = s; r.m[2][2] = c;
    return r;
}

static inline mat4 mat4_rotation_y(float a) {
    mat4 r = mat4_identity();
    float c = cosf(a), s = sinf(a);
    r.m[0][0] = c; r.m[0][2] = s;
    r.m[2][0] = -s; r.m[2][2] = c;
    return r;
}

static inline mat4 mat4_rotation_z(float a) {
    mat4 r = mat4_identity();
    float c = cosf(a), s = sinf(a);
    r.m[0][0] = c; r.m[0][1] = -s;
    r.m[1][0] = s; r.m[1][1] = c;
    return r;
}


static inline float radians(float degrees) {
    return degrees * 3.14159265358979323846f / 180.0f;
}

static inline mat4 mat4_perspective(float fov_y_radians, float aspect, float near, float far) {

    float y_scale = 1.0f / tanf(fov_y_radians * 0.5f);
    float x_scale = y_scale / aspect;

    float z_map_a = (far + near) / (near - far);
    float z_map_b = (2.0f * far * near) / (near - far);

    mat4 m = { {{0}} };
    m.m[0][0] = x_scale;
    m.m[1][1] = y_scale;
    m.m[2][2] = z_map_a;
    m.m[2][3] = z_map_b;
    m.m[3][2] = -1.0f; 
    return m;
}

static inline mat4 mat4_look_at(vec3 eye, vec3 target, vec3 up) {

    vec3 forward = vec3_normalize(vec3_sub(eye, target));
    vec3 right = vec3_normalize(vec3_cross(up, forward));
    vec3 cam_up = vec3_cross(forward, right);

    mat4 m = mat4_identity();
    // Top three rows = the camera axes. Multiplying a world point by
    // these rotates it into the camera's frame.
    m.m[0][0] = right.x;   m.m[0][1] = right.y;   m.m[0][2] = right.z;
    m.m[1][0] = cam_up.x;  m.m[1][1] = cam_up.y;  m.m[1][2] = cam_up.z;
    m.m[2][0] = forward.x; m.m[2][1] = forward.y; m.m[2][2] = forward.z;
    // Last column = move the world so the eye lands at the origin,
    // measured along each camera axis.
    m.m[0][3] = -vec3_dot(right, eye);
    m.m[1][3] = -vec3_dot(cam_up, eye);
    m.m[2][3] = -vec3_dot(forward, eye);
    return m;
}

static inline vec3 mat4_apply_point(mat4 a, vec3 p) {
    float x = a.m[0][0] * p.x + a.m[0][1] * p.y + a.m[0][2] * p.z + a.m[0][3];
    float y = a.m[1][0] * p.x + a.m[1][1] * p.y + a.m[1][2] * p.z + a.m[1][3];
    float z = a.m[2][0] * p.x + a.m[2][1] * p.y + a.m[2][2] * p.z + a.m[2][3];
    return vec3_make(x, y, z);
}