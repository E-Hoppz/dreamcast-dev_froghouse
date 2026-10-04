#ifndef FROG_COLOR_H
#define FROG_COLOR_H

#include <stdint.h>
#include "world_clock.h"

/**
 * [THE PIGMENT RAILS]
 * Hardware-accelerated matrix presets for different materials and eras.
 */
typedef enum {
    PIGMENT_DENIM,       // Raw, heavy, desaturated (Couture style)
    PIGMENT_NEON,        // High-voltage arcade output (Identity matrix)
    PIGMENT_OIL_PAINT,   // Warm, thick, Diddy Kong Racing saturation
    PIGMENT_NEMO,        // Liquid, cyan-biased, Pixar-grade glow
    PIGMENT_SATURN_VDP2, // Oxidized copper, amber & verdigris
    PIGMENT_GLIDE_DAC,   // Warm cream highlights & cold indigo shadows
    PIGMENT_FMV_SGI      // High-contrast Silicon Graphics render
} FrogPigment;

// --- [ THE COLOR ENGINE CORE ] ---

/** 
 * [POWER ON]
 * Initializes the hardware pigment matrices.
 */
void frog_color_init(void);

/**
 * [THE TUNER]
 * warmth:     1.0 (Neutral) to 2.0 (Deep Gold bias)
 * saturation: 1.0 (Neutral) to 2.0 (High-energy pop)
 */
void frog_color_set_params(float warmth, float saturation);

/**
 * [THE MODE SWITCH]
 * Flips the hardware relay to a different pigment style.
 */
void frog_color_set_pigment(FrogPigment pigment);

/**
 * [THE MASTER VIBE SHUNT]
 * Runs the color through the SH-4 XMTRX unit and injects AICA resonance.
 */
uint32_t frog_color_apply_vibe(uint32_t base_color, float z_depth, WorldClock *wc, int active_contract);

/**
 * [THE HOISTED SH-4 XMTRX STAGE]
 * Pre-loads the selected color grading matrix into the SH-4 registers and configures 
 * dither settings once per mesh draw rather than on every vertex, preventing bus stalls.
 */
void frog_color_prepare_vibe(int active_contract);

/**
 * [THE FAST PIPELINE VERTEXT GRADIST]
 * Fast color grading routine. Bypasses G2 register writes and matrix reloading inside 
 * rendering loops, relying strictly on pre-cached XMTRX register transformations.
 */
uint32_t frog_color_apply_vibe_optimized(uint32_t base_color, float z_depth, WorldClock *wc);

/**
 * [THE STROKE SHUNT]
 * Uses SH-4 Hardware Trig (FSCA) to jitter vertices based on Treble.
 */
void frog_color_calculate_stroke(float *x, float *y, WorldClock *wc, float intensity);

// Writes to the PowerVR register base directly to control the physical dithering circuit [REPAIRMAN'S SHUNT]
void pvr_set_dither(bool enable);

#endif
