#ifndef FROG_MATH_3D_H
#define FROG_MATH_3D_H

#include <math.h>

/**
 * [THE 3D SIGNAL RAIL]
 * SH-4 XMTRX unit requires 4-float alignment (16 bytes).
 * We align to 32 bytes to ensure it never crosses a cache line.
 */
typedef struct {
    float x, y, z, w; // W is the hardware padding (set to 0.0)
} __attribute__((aligned(32))) vec3_t;

// --- [ BASIC COUPLING ] ---
vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_sub(vec3_t a, vec3_t b);
vec3_t vec3_scale(vec3_t v, float s);

// --- [ LOGIC GATES ] ---
float  vec3_dot(vec3_t a, vec3_t b);
vec3_t vec3_cross(vec3_t a, vec3_t b);

// --- [ POWER SAVERS ] ---
float  vec3_length_sq(vec3_t v);
float  vec3_dist_sq(vec3_t a, vec3_t b);
float  vec3_length(vec3_t v);
vec3_t vec3_normalize(vec3_t v);

#endif
