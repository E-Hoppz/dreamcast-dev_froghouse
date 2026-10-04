//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "world_clock.h"
#include "frog_audio.h"
#include "frog_util.h"

WorldClock wc;

#ifndef SND_STREAM_BUFFER_MAX
#define SND_STREAM_BUFFER_MAX 65536
#endif

#define AICA_BUFFER_SIZE_SAMPLES (SND_STREAM_BUFFER_MAX / 4)

// Access global G2 bus lock status to prevent register-query hangs during GD-ROM streaming
extern volatile bool g_g2_bus_locked;

ModSource g_mod_matrix[MOD_DEST_COUNT] = {
    MOD_SRC_SUB_BASS,    // MOD_DEST_LIGHT_GAIN default
    MOD_SRC_TREBLE,      // MOD_DEST_STROKE_JITTER default
    MOD_SRC_SUB_BASS,    // MOD_DEST_VMU_BAR default
    MOD_SRC_SUB_BASS     // MOD_DEST_MESH_DISPLACEMENT default
};

void world_clock_init(WorldClock *wc) {
    // Clear SH-4 FPU exception trapping bits (bits 11-15 of FPSCR)
    // to prevent division-by-zero halts during fast 3D physics sweeps.
    unsigned int fpscr;
    __asm__ __volatile__("sts fpscr, %0" : "=r" (fpscr));
    fpscr &= ~0x0000f800; // Disable SH-4 FPU Exception Trapping
    __asm__ __volatile__("lds %0, fpscr" : : "r" (fpscr));

    if (g_g2_bus_locked) {
        wc->audio.sample_count = 0;
    } else {
        wc->audio.sample_count = frog_audio_get_position(GHOST_CH_WAVE_ALPHA);
    }
    
    wc->audio.monotonic_samples = 0; // Initialize continuous sample count
    wc->audio.current_offset = wc->audio.sample_count;
    wc->delta_time = 0.0f;
    
    // Initialize the Telemetry Rails
    wc->sub_bass = 0.0f;
    wc->mids = 0.0f;
    wc->treble = 0.0f;
    wc->master_peak = 0.0f;
    wc->pan_bias = 0.0f; 
    wc->mid_energy = 0.0f;
    wc->side_energy = 0.0f;
    wc->engine_playhead_cache = wc->audio.sample_count;
    wc->wheel_playhead_cache = wc->audio.sample_count;

    // Initialize gates to inactive
    for (int i = 0; i < 4; i++) {
        wc->gates[i].source = MOD_SRC_NONE;
        wc->gates[i].threshold = 0.0f;
        wc->gates[i].hysteresis = 0.0f;
        wc->gates[i].is_open = false;
        wc->gates[i].on_trigger = NULL;
    }
    
    frog_util_log("WORLD CLOCK: SPU-Slaved Motherboard Online (Hysteresis Gates active).");
}

void world_clock_update(WorldClock *wc) {
    // Reorganize loop variables inside local CPU registers to speed up pipeline execution
    register uint32_t new_sample;
    register uint32_t delta;

    // --- [ G2 BUS DEADLOCK PREVENTER ] ---
    // If the GD-ROM is reading sectors, we completely bypass reading physical registers 
    // over G2 and fall back to estimated calculations to prevent a hard lockup.
    if (g_g2_bus_locked) {
        delta = 735; // Fallback to steady 60Hz update rate (44100 / 60)
        new_sample = (wc->audio.sample_count + delta) % AICA_BUFFER_SIZE_SAMPLES;
        wc->engine_playhead_cache = new_sample;
        wc->wheel_playhead_cache = new_sample;
    } else {
        // Safe to poll physical registers over G2
        wc->engine_playhead_cache = frog_audio_get_position(GHOST_CH_WAVE_ALPHA);
        wc->wheel_playhead_cache = frog_audio_get_position(GHOST_CH_WAVE_BETA);

        new_sample = wc->engine_playhead_cache;
        
        if (new_sample >= wc->audio.sample_count) {
            delta = new_sample - wc->audio.sample_count;
        } else {
            delta = (AICA_BUFFER_SIZE_SAMPLES - wc->audio.sample_count) + new_sample;
        }

        // Handle possible audio stream pauses or latency hiccups
        if (delta == 0) {
            delta = 735; 
        }
    }

    wc->audio.sample_count = new_sample;
    wc->audio.monotonic_samples += delta;
    wc->delta_time = (float)delta * 0.00002267573f; // Replaced division with reciprocal multiplication

    // Update Telemetry Gates sequentially to optimize cache locality
    for (register int i = 0; i < 4; i++) {
        register FrogTelemetryGate *gate = &wc->gates[i];
        if (gate->source == MOD_SRC_NONE) continue;
        
        register float val = 0.0f;
        switch (gate->source) {
            case MOD_SRC_SUB_BASS:    val = wc->sub_bass; break;
            case MOD_SRC_MIDS:        val = wc->mids; break;
            case MOD_SRC_TREBLE:      val = wc->treble; break;
            case MOD_SRC_MASTER_PEAK: val = wc->master_peak; break;
            case MOD_SRC_MID_CENTER:  val = wc->mid_energy; break;
            case MOD_SRC_SIDE_WIDTH:  val = wc->side_energy; break;
            default:                  break;
        }
        
        register bool was_open = gate->is_open;
        if (!gate->is_open && val > gate->threshold) {
            gate->is_open = true;
            if (!was_open && gate->on_trigger != NULL) {
                gate->on_trigger(wc, true);
            }
        } else if (gate->is_open && val < (gate->threshold - gate->hysteresis)) {
            gate->is_open = false;
            if (was_open && gate->on_trigger != NULL) {
                gate->on_trigger(wc, false);
            }
        }
    }
}

void world_clock_configure_gate(WorldClock *wc, int gate_idx, ModSource source, float threshold, float hysteresis, FrogGateCallback callback) {
    if (wc && gate_idx >= 0 && gate_idx < 4) {
        wc->gates[gate_idx].source = source;
        wc->gates[gate_idx].threshold = threshold;
        wc->gates[gate_idx].hysteresis = hysteresis;
        wc->gates[gate_idx].is_open = false;
        wc->gates[gate_idx].on_trigger = callback;
    }
}
