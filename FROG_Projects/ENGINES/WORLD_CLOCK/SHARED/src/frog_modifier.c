#include "frog_modifier.h"
#include "frog_sq.h"
#include "frog_util.h"
#include <dc/pvr.h>
#include <dc/sq.h>

// Initialize the global experimental toggle to false (disabled on boot for safety)
bool g_pvr_direct_blend_feedback = false;

void frog_modifier_init(FrogModifier *mod, FrogCsgMode csg_mode) {
    if (!mod) return;

    uint32_t pvr_mode;
    
    // [FRONTIER 1 BLEND SHUNT]
    // If the experimental on-chip accumulation loop is active, we override the normal CSG
    // modes and configure the modifier volume to act as a hardware inclusion mask.
    if (g_pvr_direct_blend_feedback) {
        // --- [ CORRECTION: Aligned with KOS hardware-defined modifier volume modes ] ---
        pvr_mode = PVR_MODIFIER_INCLUDE_LAST_POLY;
    } else {
        switch (csg_mode) {
            case MOD_CSG_INCLUDE:
                // --- [ CORRECTION: Aligned with KOS hardware-defined modifier volume modes ] ---
                pvr_mode = PVR_MODIFIER_INCLUDE_LAST_POLY;
                break;
            case MOD_CSG_EXCLUDE:
                // --- [ CORRECTION: Aligned with KOS hardware-defined modifier volume modes ] ---
                pvr_mode = PVR_MODIFIER_EXCLUDE_LAST_POLY;
                break;
            default:
                pvr_mode = PVR_MODIFIER_NORMAL;
                break;
        }
    }

    // Compile the PowerVR2's hardware stencil parameters
    pvr_mod_compile(&mod->header, 
                    PVR_LIST_OP_MOD, 
                    pvr_mode, 
                    PVR_CULLING_NONE);
    
    if (g_pvr_direct_blend_feedback) {
        frog_util_log("MODIFIER: [FRONTIER 1] On-Chip Tile-Buffer Color Accumulator Active.");
    } else {
        frog_util_log("MODIFIER: Hardware shadow/CSG volume energized.");
    }
}

void frog_modifier_render(FrogModifier *mod) {
    if (!mod || mod->volume_count == 0) return;

    pvr_prim(&mod->header, sizeof(pvr_mod_hdr_t));

    for (uint32_t i = 0; i < mod->volume_count; i++) {
        frog_sq_shunt((void*)PVR_TA_INPUT, &mod->volumes[i], sizeof(FrogModifierVol));
    }
}

void frog_modifier_configure_blend_feedback(bool enable) {
    g_pvr_direct_blend_feedback = enable;
    printf("MODIFIER: On-Chip Tile-Buffer Blend Feedback set to: %s\n", enable ? "ENABLED" : "DISABLED");
}
