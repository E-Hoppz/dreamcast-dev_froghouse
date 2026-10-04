#ifndef HOST_COMPAT_DC_MATRIX_H
#define HOST_COMPAT_DC_MATRIX_H

#include <string.h>

// Matches the exact KallistiOS 4x4 matrix definition
typedef float matrix_t[4][4];

typedef struct {
    float m[4][4];
} host_xmtrx_t;

extern host_xmtrx_t host_xmtrx;

static inline void mat_load(const void *matrix) {
    memcpy(&host_xmtrx, matrix, sizeof(host_xmtrx_t));
}

static inline void mat_store(void *matrix) {
    memcpy(matrix, &host_xmtrx, sizeof(host_xmtrx_t));
}

static inline void mat_identity(void) {
    memset(&host_xmtrx, 0, sizeof(host_xmtrx_t));
    host_xmtrx.m[0][0] = 1.0f;
    host_xmtrx.m[1][1] = 1.0f;
    host_xmtrx.m[2][2] = 1.0f;
    host_xmtrx.m[3][3] = 1.0f;
}

// Simulates matrix multiplication: xmtrx = xmtrx * src
static inline void mat_apply(const void *src) {
    host_xmtrx_t temp;
    const float (*s)[4] = (const float (*)[4])src;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp.m[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                temp.m[i][j] += host_xmtrx.m[i][k] * s[k][j];
            }
        }
    }
    host_xmtrx = temp;
}

// Simulates in-place coordinate projection with perspective division (Z = 1/W)
#define mat_trans_single(x, y, z) do { \
    float _x = (x), _y = (y), _z = (z); \
    float xr = _x * host_xmtrx.m[0][0] + _y * host_xmtrx.m[1][0] + _z * host_xmtrx.m[2][0] + host_xmtrx.m[3][0]; \
    float yr = _x * host_xmtrx.m[0][1] + _y * host_xmtrx.m[1][1] + _z * host_xmtrx.m[2][1] + host_xmtrx.m[3][1]; \
    float zr = _x * host_xmtrx.m[0][2] + _y * host_xmtrx.m[1][2] + _z * host_xmtrx.m[2][2] + host_xmtrx.m[3][2]; \
    float wr = _x * host_xmtrx.m[0][3] + _y * host_xmtrx.m[1][3] + _z * host_xmtrx.m[2][3] + host_xmtrx.m[3][3]; \
    float inv_w = (wr != 0.0f) ? (1.0f / wr) : 1.0f; \
    (x) = xr * inv_w; \
    (y) = yr * inv_w; \
    (z) = inv_w; \
} while(0)

// Simulates in-place vector multiplication with no division
#define mat_trans_single3_nodiv(r, g, b) do { \
    float _r = (r), _g = (g), _b = (b); \
    (r) = _r * host_xmtrx.m[0][0] + _g * host_xmtrx.m[1][0] + _b * host_xmtrx.m[2][0] + host_xmtrx.m[3][0]; \
    (g) = _r * host_xmtrx.m[0][1] + _g * host_xmtrx.m[1][1] + _b * host_xmtrx.m[2][1] + host_xmtrx.m[3][1]; \
    (b) = _r * host_xmtrx.m[0][2] + _g * host_xmtrx.m[1][2] + _b * host_xmtrx.m[2][2] + host_xmtrx.m[3][2]; \
} while(0)

#endif
