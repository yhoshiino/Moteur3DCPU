// ============================================================
//  particles.cpp — the code behind particles.h
// ------------------------------------------------------------
//  Every frame:
//    1. UPDATE: reduce its life, pull it down (gravity changes
//       the vertical speed), then move it (position += speed * dt).
//    2. When its life is over, it is reborn at the emitter with
//       a new random speed.
//    3. DRAW: write a small square of pixels in the framebuffer,
//       dimmer as the particle gets older.
// ============================================================
#include "particle.h"
#include <stdlib.h>
#include "color.h"

// A random float between lo and hi. (static = only used in this file)
static float particle_rand(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

// (Re)start one particle at the emitter (ex, ey), with a random speed.
// vy is negative on purpose: y grows downward on screen, so negative = up.
static void particle_spawn(Particle* p, float ex, float ey) {
    p->x = ex;
    p->y = ey;
    p->vx = particle_rand(-70.0f, 70.0f);
    p->vy = particle_rand(-320.0f, -160.0f);
    p->life = PARTICLE_LIFE;
}

// Their lives are staggered so they do not all die (and restart)
// at the same moment.
void particles_init(Particle* p, int n, float ex, float ey) {
    for (int i = 0; i < n; i++) {
        particle_spawn(&p[i], ex, ey);
        p[i].life = particle_rand(0.0f, PARTICLE_LIFE);
    }
}

void particles_update(Particle* p, int n, float dt, float ex, float ey) {
    for (int i = 0; i < n; i++) {
        p[i].life -= dt;
        if (p[i].life <= 0.0f) {            // dead: reborn at the emitter
            particle_spawn(&p[i], ex, ey);
            continue;
        }
        p[i].vy += PARTICLE_GRAVITY * dt;   // gravity changes the speed...
        p[i].x += p[i].vx * dt;             // ...and the speed changes the position
        p[i].y += p[i].vy * dt;
    }
}

// The colour fades to black as its life runs out (k goes from 1 to 0).
void particles_draw(const Particle* p, int n, uint32_t* fb, int w, int h) {
    for (int i = 0; i < n; i++) {
        float k = p[i].life / PARTICLE_LIFE;
        uint32_t color = rgb((uint8_t)(255.0f * k),
            (uint8_t)(160.0f * k),
            (uint8_t)(40.0f * k));
        int px = (int)p[i].x;
        int py = (int)p[i].y;
        for (int dy = 0; dy < PARTICLE_SIZE; dy++) {
            for (int dx = 0; dx < PARTICLE_SIZE; dx++) {
                int x = px + dx;
                int y = py + dy;
                if (x < 0 || x >= w || y < 0 || y >= h) continue;   // off screen
                fb[y * w + x] = color;
            }
        }
    }
}