#pragma once
#include "vec.h"

struct Camera {
	vec3 position = {0,0,0};
	float yaw = 0.0f;
	float pitch = 0.0f;
	float speed = 2.f;
	float sensitivity = 0.05f;
};