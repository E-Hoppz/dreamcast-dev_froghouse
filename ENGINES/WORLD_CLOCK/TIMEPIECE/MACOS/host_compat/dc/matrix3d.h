#ifndef HOST_COMPAT_DC_MATRIX3D_H
#define HOST_COMPAT_DC_MATRIX3D_H

#include <dc/vector.h>
#include <dc/matrix.h>
#include <math.h>

static inline void mat_lookat(const point_t *eye, const point_t *center, const vector_t *up) {
    float fx = center->x - eye->x;
    float fy = center->y - eye->y;
    float fz = center->z - eye->z;
    float flen = sqrtf(fx*fx + fy*fy + fz*fz);
    if (flen > 0.0001f) { fx /= flen; fy /= flen; fz /= flen; }

    float sx = fy * up->z - fz * up->y;
    float sy = fz * up->x - fx * up->z;
    float sz = fx * up->y - fy * up->x;
    float slen = sqrtf(sx*sx + sy*sy + sz*sz);
    if (slen > 0.0001f) { sx /= slen; sy /= slen; sz /= slen; }

    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    float lookat[4][4] = {
        { sx,  ux, -fx, 0.0f },
        { sy,  uy, -fy, 0.0f },
        { sz,  uz, -fz, 0.0f },
        { -(sx*eye->x + sy*eye->y + sz*eye->z),
          -(ux*eye->x + uy*eye->y + uz*eye->z),
          (fx*eye->x + fy*eye->y + fz*eye->z),
          1.0f }
    };

    mat_load(lookat);
}

#endif
