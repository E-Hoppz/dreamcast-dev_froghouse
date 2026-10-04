#ifndef FROG_DRAW_H
#define FROG_DRAW_H

#include <stdint.h>
#include <stdbool.h>
#include "frog_asset.h"

// Forward declaration to prevent the circular include loop
struct WorldClock;

/**
 * [THE PVR VIBE RAILS]
 * Fixed hardware signal paths for consistent performance.
 */
typedef enum {
    VIBE_SONIC,   // Smooth 3D (Bilinear, Opaque, Hazy)
    VIBE_SATURN,  // Sharp 2D (Nearest, Punch-Thru)
    VIBE_NEMO,    // Liquid Glow (Bilinear, Translucent, Specular)
    VIBE_TOON,    // Hand-Drawn (Nearest, Punch-Thru, Pure Pigment)
    VIBE_DENIM,   // Heavy Matte (Bilinear, Opaque, Color-Clamped)
    VIBE_NEON     // High-Voltage (Bilinear, Additive, Specular)
} FrogVibe;

// --- [ BOOT & CALIBRATION ] ---
void frog_draw_init();

// --- [ THE DRAWING RAILS ] ---
void frog_draw_rect(float x, float y, float w, float h, uint32_t color);
void frog_draw_rect_tr(float x, float y, float w, float h, uint32_t color); 
void frog_draw_text(float x, float y, const char *text);

/**
 * [THE SEGA SATURN QUAD SHUNT]
 * Emulates the Saturn VDP1's distorted-sprite architecture.
 */
void frog_draw_saturn_quad(float x0, float y0, float x1, float y1,
                           float x2, float y2, float x3, float y3,
                           frog_texture_t *tex, uint32_t color);

/**
 * [THE HARDWARE UV SPRITE BLITTER]
 * Blasts individual cells of a texture sheet (like a font map) using custom UV coordinates.
 */
void frog_draw_sprite_uv(float x, float y, float w, float h, 
                         float u0, float v0, float u1, float v1,
                         frog_texture_t *tex, struct WorldClock *wc, 
                         uint32_t color, FrogVibe vibe);

/**
 * [THE OVERDRIVE SHUNT]
 * The hardware Firehose. Uses KOS Direct Rendering store queue structures 
 * to blast vertices directly to the PowerVR Tile Accelerator.
 */
void frog_draw_sprite_rapid(float x, float y, float w, float h, 
                            frog_texture_t *tex, 
                            struct WorldClock *wc, 
                            uint32_t color, 
                            FrogVibe vibe);

#endif
