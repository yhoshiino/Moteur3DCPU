// ============================================================
//  particles.h — a very simple pixel particle system
// ------------------------------------------------------------
//  A particle is just a point with a position, a speed and a
//  remaining life. See particles.cpp for how it moves and is drawn.
//  No 3D here: particles live directly in screen pixels, and are
//  drawn on top of the 3D scene (no depth test).
// ============================================================
#pragma once
#include <stdint.h>

#define MAX_PARTICLES     400
#define PARTICLE_LIFE     2.0f      // life of a particle, in seconds
#define PARTICLE_GRAVITY  220.0f    // pixels per second, per second
#define PARTICLE_SIZE     2         // side of the drawn square, in pixels

typedef struct {
    float x, y;      // position, in pixels
    float vx, vy;    // speed, in pixels per second
    float life;      // seconds left before it dies
} Particle;

// Create all the particles at the emitter (ex, ey), with staggered lives.
void particles_init(Particle* p, int n, float ex, float ey);

// Move every particle by dt seconds; a dead particle is reborn at the emitter.
void particles_update(Particle* p, int n, float dt, float ex, float ey);

// Write every particle in the framebuffer (w x h pixels).
void particles_draw(const Particle* p, int n, uint32_t* fb, int w, int h);