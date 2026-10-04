#ifndef FROG_SH_LIGHT_H
#define FROG_SH_LIGHT_H

#include <stdint.h>
#include "frog_math_3d.h"

// 2nd-Order Spherical Harmonics Probe: 9 float coefficients per RGB channel (27 floats total)
typedef struct {
    float  coeff_r[9];
    float  coeff_g[9];
    float  coeff_b[9];
    vec3_t position;
} FrogLightProbe;

typedef struct {
    FrogLightProbe *probes;
    uint32_t        probe_count;
} FrogSHLightingField;

void     frog_sh_init(FrogSHLightingField *field, uint32_t max_probes);
uint32_t frog_sh_evaluate(FrogSHLightingField *field, vec3_t pos, vec3_t normal);

#endif
