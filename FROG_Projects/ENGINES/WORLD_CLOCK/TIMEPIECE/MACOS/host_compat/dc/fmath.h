#ifndef HOST_COMPAT_FMATH_H
#define HOST_COMPAT_FMATH_H

#include <math.h>

// Mock SH-4 single-cycle Floating-Point Reciprocal Square Root
static inline float fsrra(float x) {
    return 1.0f / sqrtf(x);
}

// Mock SH-4 Floating-Point Sine / Cosine via fast standard lookups
static inline float fsin(float x) {
    return sinf(x);
}

static inline float fcos(float x) {
    return cosf(x);
}

// Mock SH-4 Inner Product (Dot Product)
static inline float fipr(float x, float y, float z, float w, 
                         float a, float b, float c, float d) {
    return (x * a) + (y * b) + (z * c) + (w * d);
}

#endif
