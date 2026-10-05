//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "worldclock_forall.h"
#include "world_clock.h"
#include "frog_config.h"
#include "frog_color.h"
#include "frog_clockwork_bridge.h"
#include <stdlib.h>

// This global instance is mapped directly inside libworldclock.a
extern WorldClock wc;

WorldClockOpaque* world_clock_engine_init(void) {
    // 1. Initialize configuration options
    frog_config_init();
    
    // 2. Globally force the Glide DAC hardware preset and dither register
    frog_engine_config.active_contract = CONTRACT_GLIDE;
    frog_color_init();
    frog_color_set_pigment(PIGMENT_GLIDE_DAC);
    pvr_set_dither(true);

    // 3. Start KOS drivers and SPU channels safely
    frog_audio_init(); 
    
    // 4. Initialize main clockwork motherboard
    world_clock_init(&wc);
    
    return (WorldClockOpaque*)&wc;
}

void world_clock_engine_inject(WorldClockOpaque *wc_ptr, WorldClockBridgeTarget *target) {
    if (!wc_ptr || !target) return;

    // Convert the public target pointers into private clockwork targets
    FrogClockworkTarget bridge_target;
    bridge_target.engine_delta_time = target->engine_delta_time;
    bridge_target.engine_total_ticks = target->engine_total_ticks;

    // Execute original time-coupling bridge logic safely inside binary
    frog_clockwork_inject(&bridge_target, (WorldClock*)wc_ptr);
}

void world_clock_engine_shutdown(WorldClockOpaque *wc_ptr) {
    // Nothing to deallocate, memory managed globally
    (void)wc_ptr;
}

#include "frog_debug.h"
#include <stdio.h>

void world_clock_engine_draw_telemetry(WorldClockOpaque *wc_ptr, int fps) {
    if (!wc_ptr) return;
    WorldClock *wc = (WorldClock*)wc_ptr;

    char buf[128];

    // Transparent diagnostic backing panel
    frog_debug_draw_rect(10, 10, 310, 150, 0x90000000);

    // Timing Diagnostics
    sprintf(buf, "[WORLD CLOCK] MONOTONIC: %u smp", (unsigned int)wc->audio.monotonic_samples);
    frog_debug_draw_text(20, 20, buf);

    sprintf(buf, "ENGINE FPS: %d | DELTA TIME: %.4f s", fps, wc->delta_time);
    frog_debug_draw_text(20, 40, buf);

    // SPU G2 Bus Memory Status
    sprintf(buf, "CH_ALPHA REG: %u | CH_BETA REG: %u", 
            (unsigned int)wc->engine_playhead_cache, 
            (unsigned int)wc->wheel_playhead_cache);
    frog_debug_draw_text(20, 60, buf);

    // Audio-Dynamic Voltages
    sprintf(buf, "SUB-BASS: %.2f | MID-ENERGY: %.2f", wc->sub_bass, wc->mid_energy);
    frog_debug_draw_text(20, 80, buf);

    sprintf(buf, "SIDE-WIDTH: %.2f | PAN BIAS: %.2f", wc->side_energy, wc->pan_bias);
    frog_debug_draw_text(20, 100, buf);
}
