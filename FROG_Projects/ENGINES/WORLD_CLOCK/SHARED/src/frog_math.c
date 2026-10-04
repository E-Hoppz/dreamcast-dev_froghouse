#include "frog_math.h"
#include <math.h>

float frog_clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

float frog_lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float frog_math_colani_lerp(float a, float b, float t) {
    float ft = t * t * (3.0f - 2.0f * t);
    return a + ft * (b - a);
}

float frog_math_map_signal(float val, float a, float b) {
    if (fabsf(a - b) < 0.0001f) return 0.0f;
    return frog_clamp((val - a) / (b - a), 0.0f, 1.0f);
}

float frog_math_wrap(float val, float min, float max) {
    float range = max - min;
    while (val < min) val += range;
    while (val >= max) val -= range;
    return val;
}

float frog_math_catmull_rom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                  (-p0 + p2) * t +
                  (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                  (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

bool frog_math_point_in_box(float px, float py, float bx, float by, float bw, float bh) {
    return (px >= bx && px <= (bx + bw) && py >= by && py <= (by + bh));
}
