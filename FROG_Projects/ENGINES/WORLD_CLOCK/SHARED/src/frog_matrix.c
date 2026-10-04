//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
// Optimized SH-4 Matrix pipeline utilizing the XMTRX Register bank
// and the hardware-accelerated FTRV instruction.

#include "frog_matrix.h"
#include <dc/matrix.h>  // Direct SH-4 XMTRX access
#include <math.h>
#include <string.h>

mat4_t mat4_identity() {
    mat4_t res;
    mat_identity();
    mat_store((matrix_t *)res.m); 
    return res;
}

mat4_t mat4_rotate_y(float angle) {
    mat4_t res;
    
    // Clear back registers and load identity natively
    mat_identity();
    
    float s = sinf(angle);
    float c = cosf(angle);

    // Dreamcast Hardware Matrix is Column-Major.
    // We construct the rotation directly into the current matrix.
    matrix_t temp = {
        { c,    0.0f, -s,   0.0f },
        { 0.0f, 1.0f,  0.0f,0.0f },
        { s,    0.0f,  c,   0.0f },
        { 0.0f, 0.0f,  0.0f,1.0f }
    };

    mat_apply(&temp);
    mat_store((matrix_t *)res.m); 

    return res;
}

mat4_t mat4_translate(float x, float y, float z) {
    mat4_t res;
    
    mat_identity();

    // The 4th column handles translation in the XMTRX unit
    matrix_t temp = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { x,    y,    z,    1.0f }
    };

    mat_apply(&temp);
    mat_store((matrix_t *)res.m);

    return res;
}

// Highly optimized matrix multiplication using direct SH-4 hardware registers
mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t res;
    
    mat_load((matrix_t *)a.m);    // Load Matrix A into the XMTRX unit
    mat_apply((matrix_t *)b.m);   // Hardware multiply by Matrix B
    mat_store((matrix_t *)res.m); // Store result back directly
    
    return res;
}

// Transforms a 3D vector by a matrix using the FTRV hardware instruction
vec3_t vec3_apply_mat4(vec3_t v, mat4_t m) {
    register float x = v.x;
    register float y = v.y;
    register float z = v.z;
    
    mat_load((matrix_t *)m.m);
    
    // Executes the single-cycle FTRV instruction on the FPU
    mat_trans_single3_nodiv(x, y, z); 
    
    return (vec3_t){x, y, z, 1.0f};
}
