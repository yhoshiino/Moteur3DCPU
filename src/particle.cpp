#include "particle.h"
#include <cstdlib>
#include "color.h"

static float particle_rand(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

static void particle_spawn(Particle* p, float ex, float ey) {
    p->x = ex;
    p->y = ey;
    p->vx = particle_rand(-70.0f, 70.0f);
    p->vy = particle_rand(-320.0f, -160.0f);
    p->life = PARTICLE_LIFE;
}

void particles_init(Particle* p, int n, float ex, float ey) {
    for (int i = 0; i < n; i++) {
        particle_spawn(&p[i], ex, ey);
        p[i].life = particle_rand(0.0f, PARTICLE_LIFE);
    }
}

void particles_update(Particle* p, int n, float dt, float ex, float ey) {
    for (int i = 0; i < n; i++) {
        p[i].life -= dt;
        if (p[i].life <= 0.0f) {
            particle_spawn(&p[i], ex, ey);
            continue;
        }

        p[i].vy += PARTICLE_GRAVITY * dt;
        p[i].x += p[i].vx * dt;
        p[i].y += p[i].vy * dt;
    }
}

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
                if (x < 0 || x >= w || y < 0 || y >= h) continue;
                fb[y * w + x] = color;
            }
        }
    }
}