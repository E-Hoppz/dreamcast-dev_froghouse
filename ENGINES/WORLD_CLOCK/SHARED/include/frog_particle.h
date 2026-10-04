#ifndef FROG_PARTICLE_H
#define FROG_PARTICLE_H
#include <stdint.h>

typedef struct {
    float x, y, z;
    float vx, vy, vz;
    int life;
    uint32_t color;
    int active;
} FrogParticle;

void frog_particle_init();
void frog_particle_spawn(float x, float y, float z, uint32_t color);
void frog_particle_update();
void frog_particle_draw();

#endif
