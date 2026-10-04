#ifndef FROG_MATRIX_H
#define FROG_MATRIX_H

#include "frog_math_3d.h"

// [REPAIRMAN'S TRUTH] 
// We use __attribute__((aligned(32))) to ensure this struct 
// sits perfectly on the SH-4 hardware bus lines.
typedef struct {
    float m[4][4];
} __attribute__((aligned(32))) mat4_t;

// --- [ GENERATORS ] ---
mat4_t mat4_identity();
mat4_t mat4_translate(float x, float y, float z);
mat4_t mat4_rotate_y(float angle);

// --- [ HARDWARE OPS ] ---
mat4_t mat4_mul(mat4_t a, mat4_t b);

// Transforms a 3D Vector by a Matrix using the XMTRX Unit
vec3_t vec3_apply_mat4(vec3_t v, mat4_t m);

#endif
