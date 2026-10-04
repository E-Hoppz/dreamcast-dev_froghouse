#ifndef FROG_MODIFIER_H
#define FROG_MODIFIER_H

#include <stdint.h>
#include <stdbool.h>
#include <dc/pvr.h>

/**
 * [THE GPU-CSG MODES]
 * Hardware-accelerated boolean volume operations.
 */
typedef enum {
    MOD_CSG_NORMAL,  // Standard shadow volume stencil
    MOD_CSG_INCLUDE, // Include volume (only render pixels inside the hull)
    MOD_CSG_EXCLUDE  // Exclude volume (subtract/carve out pixels inside the hull)
} FrogCsgMode;

typedef struct {
    uint32_t flags;      
    float    ax, ay, az; 
    float    bx, by, bz; 
    float    cx, cy, cz; 
    uint32_t padding[6]; 
} __attribute__((aligned(32))) FrogModifierVol;

typedef struct {
    FrogModifierVol *volumes;
    uint32_t        volume_count;
    pvr_mod_hdr_t   header;
} FrogModifier;

// [FRONTIER 1 EXPERIMENTAL TOGGLE]
// Globally accessible toggle to enable/disable On-Chip Tile-Buffer Color Accumulation.
// Defaults to false (standard culling) for safety.
extern bool g_pvr_direct_blend_feedback;

// Initialize modifier volume with standard, CSG, or blend-feedback parameters
void frog_modifier_init(FrogModifier *mod, FrogCsgMode csg_mode);
void frog_modifier_render(FrogModifier *mod);

// [NEW] Configure the hardware blend feedback loop state on the fly
void frog_modifier_configure_blend_feedback(bool enable);

#endif
