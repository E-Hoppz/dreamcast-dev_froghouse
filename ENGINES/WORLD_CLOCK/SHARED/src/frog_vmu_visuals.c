#include "frog_vmu_visuals.h"
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <string.h>

static uint32_t last_vmu_update = 0;
static float vmu_threshold = 0.7f;

static maple_device_t *cached_vmu = NULL;

void frog_vmu_vibe_sync(WorldClock *wc, const uint8_t *icon_on, const uint8_t *icon_off) {
    if (!wc) return;

    // 1. MAPLE BUS PROTECTION
    if (wc->audio.sample_count - last_vmu_update < 2205) return; 

    // 2. CHECK OR SCAN COUPLING
    if (!cached_vmu) {
        cached_vmu = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
    }

    if (!cached_vmu) return;

    // --- [ JOINT 5: VMU LIVE LCD AUDIO VU METER ] ---
    // Instead of displaying a static image, we dynamically write a 
    // real-time bouncing horizontal bar graph mapped to wc->sub_bass
    uint8_t vu_buffer[192]; // 48x32 monochrome LCD buffer (192 bytes)
    memset(vu_buffer, 0, sizeof(vu_buffer));

    // Determine bar length based on sub-bass voltage (0 to 48 pixels)
 float vmu_energy = world_clock_get_modulated(wc, MOD_DEST_VMU_BAR);
            int bar_length = (int)(vmu_energy * 48.0f);
            if (bar_length > 48) bar_length = 48;
            if (bar_length < 0)  bar_length = 0;

    // Draw horizontal bars inside the 192-byte frame buffer
    for (int y = 12; y < 20; y++) { // Center the VU meter vertically
        for (int x = 0; x < bar_length; x++) {
            int byte_idx = (y * 6) + (x / 8);
            int bit_idx = 7 - (x % 8);
            vu_buffer[byte_idx] |= (1 << bit_idx);
        }
    }

    // Draw frame boundaries
    for (int x = 0; x < 48; x++) {
        vu_buffer[x / 8] |= (1 << (7 - (x % 8)));          // Top line
        vu_buffer[186 + (x / 8)] |= (1 << (7 - (x % 8)));  // Bottom line
    }

    // Shunt live frames directly to the Maple card LCD
    vmu_draw_lcd(cached_vmu, vu_buffer);
    last_vmu_update = wc->audio.sample_count;
}

void frog_vmu_set_sensitivity(float val) {
    vmu_threshold = val;
}
