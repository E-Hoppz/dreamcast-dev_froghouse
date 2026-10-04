#include "frog_particle.h"
#include "frog_draw.h"
#include "frog_util.h"

#define MAX_PARTICLES 256
static FrogParticle pool[MAX_PARTICLES];

void frog_particle_init() {
    for(int i=0; i<MAX_PARTICLES; i++) pool[i].active = 0;
}

void frog_particle_spawn(float x, float y, float z, uint32_t color) {
    for(int i=0; i<MAX_PARTICLES; i++) {
        if(!pool[i].active) {
            pool[i].active = 1;
            pool[i].x = x; pool[i].y = y; pool[i].z = z;
            pool[i].vx = frog_rand_range(-0.5f, 0.5f);
            pool[i].vy = frog_rand_range(0.1f, 0.8f); 
            pool[i].life = 60; 
            pool[i].color = color;
            return;
        }
    }
}

void frog_particle_update() {
    for(int i=0; i<MAX_PARTICLES; i++) {
        if(pool[i].active) {
            pool[i].x += pool[i].vx;
            pool[i].y += pool[i].vy;
            // Downward acceleration simulating gravitational force
            pool[i].vy -= 0.02f; 
            pool[i].life--;
            if(pool[i].life <= 0) {
                pool[i].active = 0;
            }
        }
    }
}

void frog_particle_draw() {
    for(int i=0; i<MAX_PARTICLES; i++) {
        if(pool[i].active) {
            // Draw a basic flat quad to represent our dynamic particle
            frog_draw_rect(pool[i].x, pool[i].y, 4.0f, 4.0f, pool[i].color);
        }
    }
}
