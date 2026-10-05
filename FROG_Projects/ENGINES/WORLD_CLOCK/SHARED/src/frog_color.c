#include "frog_color.h"
#include "frog_matrix.h"
#include "frog_math.h"
#include "frog_air_signal.h"
#include "frog_config.h" 
#include <dc/matrix.h>
#include <dc/fmath.h> 
#include <dc/pvr.h>
#include <math.h>

static float cfg_warmth = 1.2f;
static float cfg_saturation = 1.4f;
static mat4_t active_matrix;
static mat4_t denim_mat, neon_mat, oil_mat, nemo_mat, saturn_vdp2_mat, glide_dac_mat, fmv_sgi_mat;
static FrogAirDensity color_saturator = {0.0f};

/**
 * [THE FILMIC LIMITER]
 * Uses the SH-4 FSCA (fsin) hardware to roll off color peaks.
 */
static uint32_t frog_color_soft_clip(float r, float g, float b, float a, WorldClock *wc, int active_contract) {
    (void)wc;
    (void)active_contract;

    r = fsin(frog_clamp(r, 0.0f, 1.0f) * 1.5707f); 
    g = fsin(frog_clamp(g, 0.0f, 1.0f) * 1.5707f);
    b = fsin(frog_clamp(b, 0.0f, 1.0f) * 1.5707f);

    return ((uint32_t)(a * 255) << 24) | 
           ((uint32_t)(r * 255) << 16) | 
           ((uint32_t)(g * 255) << 8)  | 
            (uint32_t)(b * 255);
}

/**
 * [THE HARDWARE OVERCLOCK: FB DITHER SET]
 * Writes directly to the physical post-processing registers inside the PowerVR chip (PVR_FB_CFG_2).
 */
void pvr_set_dither(bool enable) {
    volatile uint32_t *fb_cfg_2 = (volatile uint32_t *)(0xa05f8000 + PVR_FB_CFG_2);
    if (enable) {
        *fb_cfg_2 |= (1 << 3);  // Enable hardware-level post-filtering dither patterns
    } else {
        *fb_cfg_2 &= ~(1 << 3); // Disable hardware dither
    }
}

void frog_color_init() {
    denim_mat = mat4_identity();
    denim_mat.m[0][0] = 0.6f; denim_mat.m[1][1] = 0.6f; denim_mat.m[2][2] = 0.7f;

    oil_mat = mat4_identity();
    oil_mat.m[0][0] = 1.5f; oil_mat.m[1][1] = 1.2f; oil_mat.m[2][2] = 0.8f;

    nemo_mat = mat4_identity();
    nemo_mat.m[0][0] = 0.8f; nemo_mat.m[1][1] = 1.4f; nemo_mat.m[2][2] = 1.6f;

    // NEON: Dynamic, high-voltage neon glow shift (High Pink/Cyan boost)
    neon_mat = mat4_identity();
    neon_mat.m[0][0] = 1.40f; neon_mat.m[0][1] = 0.00f; neon_mat.m[0][2] = 0.40f; 
    neon_mat.m[1][0] = 0.20f; neon_mat.m[1][1] = 1.30f; neon_mat.m[1][2] = 0.00f; 
    neon_mat.m[2][0] = 0.40f; neon_mat.m[2][1] = 0.00f; neon_mat.m[2][2] = 1.60f; 

    // SATURN VDP2: Copper-Oxide (Oxidized copper greens & ambers)
    saturn_vdp2_mat = mat4_identity();
    saturn_vdp2_mat.m[0][0] = 1.20f; saturn_vdp2_mat.m[0][1] = 0.20f; saturn_vdp2_mat.m[0][2] = 0.00f; 
    saturn_vdp2_mat.m[1][0] = 0.10f; saturn_vdp2_mat.m[1][1] = 1.10f; saturn_vdp2_mat.m[1][2] = 0.30f; 
    saturn_vdp2_mat.m[2][0] = 0.00f; saturn_vdp2_mat.m[2][1] = 0.30f; saturn_vdp2_mat.m[2][2] = 0.80f; 

    // GLIDE DAC: 3dfx Warm analog cream/indigo shadows
    glide_dac_mat = mat4_identity();
    glide_dac_mat.m[0][0] = 1.30f; glide_dac_mat.m[0][1] = 0.05f; glide_dac_mat.m[0][2] = 0.00f; 
    glide_dac_mat.m[1][0] = 0.05f; glide_dac_mat.m[1][1] = 1.15f; glide_dac_mat.m[1][2] = 0.05f; 
    glide_dac_mat.m[2][0] = 0.10f; glide_dac_mat.m[2][1] = 0.00f; glide_dac_mat.m[2][2] = 1.40f; 

    // FMV SGI: High-contrast Silicon Graphics pre-rendered look
    fmv_sgi_mat = mat4_identity();
    fmv_sgi_mat.m[0][0] = 1.40f; fmv_sgi_mat.m[0][1] = 0.00f; fmv_sgi_mat.m[0][2] = 0.15f; 
    fmv_sgi_mat.m[1][0] = 0.00f; fmv_sgi_mat.m[1][1] = 1.30f; fmv_sgi_mat.m[1][2] = 0.00f; 
    fmv_sgi_mat.m[2][0] = 0.15f; fmv_sgi_mat.m[2][1] = 0.00f; fmv_sgi_mat.m[2][2] = 1.50f; 

    active_matrix = oil_mat; 
}

void frog_color_set_params(float warmth, float saturation) {
    cfg_warmth = warmth;
    cfg_saturation = saturation;
}

void frog_color_set_pigment(FrogPigment pigment) {
    switch(pigment) {
        case PIGMENT_DENIM:       active_matrix = denim_mat; break;
        case PIGMENT_NEON:        active_matrix = neon_mat; break;
        case PIGMENT_OIL_PAINT:   active_matrix = oil_mat;   break;
        case PIGMENT_NEMO:        active_matrix = nemo_mat;  break;
        case PIGMENT_SATURN_VDP2: active_matrix = saturn_vdp2_mat; break;
        case PIGMENT_GLIDE_DAC:   active_matrix = glide_dac_mat;   break;
        case PIGMENT_FMV_SGI:     active_matrix = fmv_sgi_mat;     break;
    }
}

void frog_color_calculate_stroke(float *x, float *y, WorldClock *wc, float intensity) {
    float stroke_energy = world_clock_get_modulated(wc, MOD_DEST_STROKE_JITTER);
    float vibe = stroke_energy * intensity;
    float phase = (float)(wc->audio.sample_count % 44100) * 0.00002267573f; // Replaced division with reciprocal
    
    *x += fsin(phase * 3.14f) * vibe;
    *y += fcos(phase * 2.50f) * vibe;
}

uint32_t frog_color_apply_vibe(uint32_t base_color, float z_depth, WorldClock *wc, int active_contract) {
    float r = ((base_color >> 16) & 0xFF) * 0.00392156862f; // Replaced /255.0f with reciprocal
    float g = ((base_color >> 8)  & 0xFF) * 0.00392156862f;
    float b = ( base_color        & 0xFF) * 0.00392156862f;
    float a = ((base_color >> 24) & 0xFF) * 0.00392156862f;

    // --- [THE VOLTAGE-BIAS LIGHT MAP] ---
    float light_energy = world_clock_get_modulated(wc, MOD_DEST_LIGHT_GAIN);
    float gain = 1.0f + (light_energy * 0.4f);
    r *= gain;
    g *= gain;
    b *= gain;

    // --- [THE STEREO PAN COLOR SKEW] ---
    r += (wc->pan_bias * 0.15f);
    b -= (wc->pan_bias * 0.15f);

    // 2. THE HARDWARE XMTRX TRANSFORM
    if (active_contract == CONTRACT_GLIDE) {
        mat_load((matrix_t *)glide_dac_mat.m);
        pvr_set_dither(true);
    } else {
        mat_load((matrix_t *)active_matrix.m);
        pvr_set_dither(false);
    }
    mat_trans_single3_nodiv(r, g, b);

    // 3. ANALOG WARMTH & OVERDRIVE
    r *= (cfg_warmth * cfg_saturation);
    g *= (cfg_warmth * cfg_saturation * 0.9f); 

    // 4. RESONANCE INJECTION (Airwindows Density)
    float energy = frog_air_step_density(&color_saturator, wc->sub_bass);
    r += (energy * 0.3f);
    g += (energy * 0.2f);

    // 5. DEPTH-BIAS (The Haze Rail)
    float depth_factor = z_depth * 0.00125f; // Replaced division with reciprocal (* 1/800.0)
    if (depth_factor > 1.0f) depth_factor = 1.0f;

    r = frog_lerp(r, 0.05f, depth_factor); 
    b = frog_lerp(b, 0.50f, depth_factor);

    return frog_color_soft_clip(r, g, b, a, wc, active_contract);
}

// --- [ HIGH-PERFORMANCE HOISTED PIPELINE SQUEEZE ] ---

void frog_color_prepare_vibe(int active_contract) {
    if (active_contract == CONTRACT_GLIDE) {
        mat_load((matrix_t *)glide_dac_mat.m);
        pvr_set_dither(true);
    } else {
        mat_load((matrix_t *)active_matrix.m);
        pvr_set_dither(false);
    }
}

uint32_t frog_color_apply_vibe_optimized(uint32_t base_color, float z_depth, WorldClock *wc) {
    register float r = ((base_color >> 16) & 0xFF) * 0.00392156862f; // Replaced division with reciprocal
    register float g = ((base_color >> 8)  & 0xFF) * 0.00392156862f;
    register float b = ( base_color        & 0xFF) * 0.00392156862f;
    register float a = ((base_color >> 24) & 0xFF) * 0.00392156862f;

    // Direct voltage-bias scale
    float light_energy = world_clock_get_modulated(wc, MOD_DEST_LIGHT_GAIN);
    float gain = 1.0f + (light_energy * 0.4f);
    r *= gain;
    g *= gain;
    b *= gain;

    // Pan-bias color shift
    r += (wc->pan_bias * 0.15f);
    b -= (wc->pan_bias * 0.15f);

    // Transforms RGB vector using previously loaded registers in the XMTRX unit
    mat_trans_single3_nodiv(r, g, b);

    // Soft-saturation and warmth limits
    r *= (cfg_warmth * cfg_saturation);
    g *= (cfg_warmth * cfg_saturation * 0.9f); 

    // Dynamic bass haze calculation
    float energy = frog_air_step_density(&color_saturator, wc->sub_bass);
    r += (energy * 0.3f);
    g += (energy * 0.2f);

    // Depth-culling haze blending
    float depth_factor = z_depth * 0.00125f; // Replaced division with reciprocal (* 1/800.0)
    if (depth_factor > 1.0f) depth_factor = 1.0f;

    r = frog_lerp(r, 0.05f, depth_factor); 
    b = frog_lerp(b, 0.50f, depth_factor);

    // Sinusoidal filmic soft-clip using hardware FSCA
    r = fsin(frog_clamp(r, 0.0f, 1.0f) * 1.5707f); 
    g = fsin(frog_clamp(g, 0.0f, 1.0f) * 1.5707f);
    b = fsin(frog_clamp(b, 0.0f, 1.0f) * 1.5707f);

    return ((uint32_t)(a * 255) << 24) | 
           ((uint32_t)(r * 255) << 16) | 
           ((uint32_t)(g * 255) << 8)  | 
            (uint32_t)(b * 255);
}
