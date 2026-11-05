#pragma once
#include <stdint.h>

#define MAX_PARTICLES     400
#define PARTICLE_LIFE     2.0f 
#define PARTICLE_GRAVITY  220.0f 
#define PARTICLE_SIZE     2 

struct Particle {
    float x, y;
    float vx, vy;
    float life;
};

void particles_init(Particle* p, int n, float ex, float ey);
void particles_update(Particle* p, int n, float dt, float ex, float ey);
void particles_draw(const Particle* p, int n, uint32_t* fb, int w, int h);