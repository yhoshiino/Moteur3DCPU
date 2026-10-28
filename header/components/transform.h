#pragma once
#include "vec.h"

struct Transform {
    vec3  position = { 0, 0, 0 };
    float rotation_y = 0.0f;     // radians
    float scale = 1.0f;
};