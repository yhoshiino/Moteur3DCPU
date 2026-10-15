// ============================================================
//  mat.h — 4x4 matrices (chapter 5)
// ------------------------------------------------------------
//  Chapter 4's matrix module plus the two matrices that make a
//  3D pipeline: mat_perspective (the projection) and
//  mat_look_at (the camera/view), plus mat4_mul_vec4 which
//  keeps the w component the perspective divide needs.
//
//  Storage is row-major: m[row][col].
// ============================================================
#pragma once
#include "vec.h"
#include <math.h>

typedef struct { float m[4][4]; } mat4;

static inline mat4 mat4_identity(void) {
    mat4 r = { {{0}} };
    r.m[0][0] = 1.0f; r.m[1][1] = 1.0f; r.m[2][2] = 1.0f; r.m[3][3] = 1.0f;
    return r;
}

static inline mat4 mat4_mul(mat4 a, mat4 b) {
    mat4 r = { {{0}} };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a.m[i][k] * b.m[k][j];
            r.m[i][j] = s;
        }
    return r;
}

// Apply a matrix to a point, KEEPING the resulting w. The
// projection matrix puts depth into w, so unlike chapter 4 we
// must not discard it — the caller does the perspective divide.
static inline vec4 mat4_mul_vec4(mat4 a, vec4 v) {
    vec4 r;
    r.x = a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z + a.m[0][3] * v.w;
    r.y = a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z + a.m[1][3] * v.w;
    r.z = a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z + a.m[2][3] * v.w;
    r.w = a.m[3][0] * v.x + a.m[3][1] * v.y + a.m[3][2] * v.z + a.m[3][3] * v.w;
    return r;
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

// ---- The two new matrices for the 3D pipeline ----

// Convert an angle in degrees to radians. The trig functions
// (sinf, cosf, tanf) all expect radians, but degrees are easier to
// think in — so callers say radians(60.0f) instead of writing
// 60.0f * PI / 180.0f by hand.
static inline float radians(float degrees) {
    return degrees * 3.14159265358979323846f / 180.0f;
}

// Perspective projection matrix. Inputs:
//   fov_y_radians : vertical field of view, in radians (use radians())
//   aspect        : viewport width / height
//   near, far     : clip-plane distances (both positive, near < far)
//
// This builds exactly the matrix written out in README section 5.9.
// Each assignment below fills in one labelled entry of that matrix:
//
//     [ x_scale   0        0          0        ]
//     [ 0         y_scale  0          0        ]
//     [ 0         0        z_map_a    z_map_b  ]
//     [ 0         0        -1         0        ]
//
// Reading it: x_scale / y_scale apply the field of view (squeezing
// or spreading x and y); z_map_a / z_map_b remap depth so the near
// plane lands at -1 and the far plane at +1 after the divide; and
// the -1 in the bottom row copies -z into the output's w, which is
// the "w trick" (section 5.5) that makes the perspective divide work.
static inline mat4 mat4_perspective(float fov_y_radians, float aspect,
    float near, float far) {
    // How strongly to scale y. A narrow fov (zoomed in) gives a big
    // number; a wide fov gives a small one. Comes from the geometry
    // of the frustum's half-angle.
    float y_scale = 1.0f / tanf(fov_y_radians * 0.5f);
    float x_scale = y_scale / aspect;   // x also corrected for the window shape

    // Depth-remap coefficients (so near -> -1, far -> +1 after the divide).
    float z_map_a = (far + near) / (near - far);
    float z_map_b = (2.0f * far * near) / (near - far);

    mat4 m = { {{0}} };
    m.m[0][0] = x_scale;
    m.m[1][1] = y_scale;
    m.m[2][2] = z_map_a;
    m.m[2][3] = z_map_b;
    m.m[3][2] = -1.0f;       // out.w = -z  (loads camera depth into w)
    return m;
}

// View matrix from three inputs (README section 5.10):
//   eye    : where the camera IS (a point in the world)
//   target : the point the camera LOOKS AT (a point in the world)
//   up     : which way is "up" for the camera (a direction, e.g. (0,1,0))
//
// From those we work out the camera's own three axes, then build the
// matrix that re-expresses the world in those axes (so that, to the
// projection matrix, the camera sits at the origin looking down -z).
static inline mat4 mat4_look_at(vec3 eye, vec3 target, vec3 up) {
    // The camera's three axis directions:
    //   forward = which way the camera faces. We point it from the
    //             target back toward the eye, so it becomes the
    //             camera's +z (the camera looks down its own -z).
    //   right   = the camera's +x, perpendicular to forward and up.
    //   cam_up  = the camera's +y, made exactly perpendicular to the
    //             other two (the input "up" might be a little off).
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