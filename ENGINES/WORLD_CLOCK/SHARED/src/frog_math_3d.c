#include "frog_math_3d.h"
#include <dc/fmath.h> // [REPAIRMAN'S FIX] Native fast-math intrinsics
#include <math.h>

vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z, 0.0f};
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z, 0.0f};
}

vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s, 0.0f};
}

/**
 * [THE SH-4 FIPR SHUNT]
 * Utilizing the KallistiOS fipr intrinsic maps compiler register assignments 
 * dynamically, preventing variable spills while preserving raw instruction speed.
 */
float vec3_dot(vec3_t a, vec3_t b) {
    return fipr(a.x, a.y, a.z, 0.0f, b.x, b.y, b.z, 0.0f);
}

vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
        0.0f
    };
}

float vec3_length_sq(vec3_t v) { return vec3_dot(v, v); }
float vec3_length(vec3_t v)    { return sqrtf(vec3_length_sq(v)); }
float vec3_dist_sq(vec3_t a, vec3_t b) { return vec3_length_sq(vec3_sub(a, b)); }

vec3_t vec3_normalize(vec3_t v) {
    float len_sq = vec3_length_sq(v);
    if (len_sq > 0.00001f) {
        float inv_len = 1.0f / sqrtf(len_sq);
        return vec3_scale(v, inv_len);
    }
    return (vec3_t){0, 0, 0, 0};
}
