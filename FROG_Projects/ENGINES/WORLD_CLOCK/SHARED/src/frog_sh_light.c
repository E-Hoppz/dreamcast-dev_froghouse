#include "frog_sh_light.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void frog_sh_init(FrogSHLightingField *field, uint32_t max_probes) {
    if (field) {
        field->probes = (FrogLightProbe *)malloc(max_probes * sizeof(FrogLightProbe));
        field->probe_count = 0;
    }
}

// Low-Order 2nd-Order Spherical Harmonics Irradiance Evaluation
static void evaluate_sh_basis(vec3_t n, float *basis) {
    // 2nd-Order Spherical Harmonics basis functions (Y_lm)
    basis[0] = 0.282095f;                             // Y_00
    basis[1] = -0.488603f * n.y;                      // Y_1-1
    basis[2] = 0.488603f * n.z;                       // Y_10
    basis[3] = -0.488603f * n.x;                      // Y_11
    basis[4] = 1.092548f * n.x * n.y;                 // Y_2-2
    basis[5] = -1.092548f * n.y * n.z;                // Y_2-1
    basis[6] = 0.315392f * (3.0f * n.z * n.z - 1.0f); // Y_20
    basis[7] = -1.092548f * n.x * n.z;                // Y_21
    basis[8] = 0.546274f * (n.x * n.x - n.y * n.y);   // Y_22
}

uint32_t frog_sh_evaluate(FrogSHLightingField *field, vec3_t pos, vec3_t normal) {
    if (!field || field->probe_count == 0) return 0xFFFFFFFF;

    // Find the two nearest light probes and interpolate their SH coefficients
    uint32_t near_idx1 = 0, near_idx2 = 0;
    float min_dist_sq1 = 1e30f, min_dist_sq2 = 1e30f;

    for (uint32_t i = 0; i < field->probe_count; i++) {
        float d_sq = vec3_dist_sq(pos, field->probes[i].position);
        if (d_sq < min_dist_sq1) {
            min_dist_sq2 = min_dist_sq1;
            near_idx2 = near_idx1;
            min_dist_sq1 = d_sq;
            near_idx1 = i;
        } else if (d_sq < min_dist_sq2) {
            min_dist_sq2 = d_sq;
            near_idx2 = i;
        }
    }

    float dist1 = sqrtf(min_dist_sq1);
    float dist2 = sqrtf(min_dist_sq2);
    float total_dist = dist1 + dist2;

    float w1 = (total_dist > 0.001f) ? (dist2 / total_dist) : 1.0f;
    float w2 = 1.0f - w1;

    // Calculate basis vectors
    float basis[9];
    evaluate_sh_basis(normal, basis);

    // Reconstruct colors using fast dot products on the SH-4
    float r = 0.0f, g = 0.0f, b = 0.0f;
    FrogLightProbe *p1 = &field->probes[near_idx1];
    FrogLightProbe *p2 = &field->probes[near_idx2];

    for (int i = 0; i < 9; i++) {
        float coeff_r = p1->coeff_r[i] * w1 + p2->coeff_r[i] * w2;
        float coeff_g = p1->coeff_g[i] * w1 + p2->coeff_g[i] * w2;
        float coeff_b = p1->coeff_b[i] * w1 + p2->coeff_b[i] * w2;

        r += coeff_r * basis[i];
        g += coeff_g * basis[i];
        b += coeff_b * basis[i];
    }

    // Clamp values to prevent out-of-bounds artifacts
    uint32_t ir = (uint32_t)(fmaxf(0.0f, fminf(r, 1.0f)) * 255.0f);
    uint32_t ig = (uint32_t)(fmaxf(0.0f, fminf(g, 1.0f)) * 255.0f);
    uint32_t ib = (uint32_t)(fmaxf(0.0f, fminf(b, 1.0f)) * 255.0f);

    return (0xFF000000) | (ir << 16) | (ig << 8) | ib;
}
