//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "world_clock.h"
#include "frog_audio.h"
#include "frog_util.h"

WorldClock wc;

ModSource g_mod_matrix[MOD_DEST_COUNT] = {
    MOD_SRC_SUB_BASS,
    MOD_SRC_TREBLE,
    MOD_SRC_SUB_BASS,
    MOD_SRC_SUB_BASS
};

static uint64_t s_last_aica_samples = 0;
static bool s_clock_running = false;

void world_clock_init(WorldClock *wc) {
    unsigned int fpscr;
    __asm__ __volatile__("sts fpscr, %0" : "=r" (fpscr));
    fpscr &= ~0x0003f000;
    fpscr |= (1 << 18);
    __asm__ __volatile__("lds %0, fpscr" : : "r" (fpscr));

    uint64_t start_samples = frog_audio_get_monotonic_samples();
    
    wc->audio.monotonic_samples = start_samples;
    wc->audio.sample_count = (uint32_t)(start_samples & 0xFFFFFFFF);
    wc->audio.current_offset = wc->audio.sample_count;
    wc->delta_time = 0.0f;
    
    wc->sub_bass = 0.0f;
    wc->mids = 0.0f;
    wc->treble = 0.0f;
    wc->master_peak = 0.0f;
    wc->pan_bias = 0.0f; 
    wc->mid_energy = 0.0f;
    wc->side_energy = 0.0f;
    wc->engine_playhead_cache = wc->audio.sample_count;
    wc->wheel_playhead_cache = wc->audio.sample_count;

    for (int i = 0; i < 4; i++) {
        wc->gates[i].source = MOD_SRC_NONE;
        wc->gates[i].threshold = 0.0f;
        wc->gates[i].hysteresis = 0.0f;
        wc->gates[i].is_open = false;
        wc->gates[i].on_trigger = NULL;
    }

    s_last_aica_samples = start_samples;
    s_clock_running = true;
    
    frog_util_log("WORLD CLOCK: Inversion of Command Online. Slaved to 44.1kHz AICA Crystal.");
}

void world_clock_update(WorldClock *wc) {
    if (!s_clock_running) return;

    uint64_t current_samples = frog_audio_get_monotonic_samples();
    uint64_t delta_samples = current_samples - s_last_aica_samples;
    s_last_aica_samples = current_samples;

    if (delta_samples > 11025) {
        delta_samples = 11025;
    }

    wc->audio.monotonic_samples += delta_samples;
    wc->audio.sample_count = (uint32_t)(wc->audio.monotonic_samples & 0xFFFFFFFF);
    wc->delta_time = (float)delta_samples * 0.0000226757369f;

    wc->engine_playhead_cache = wc->audio.sample_count;
    wc->wheel_playhead_cache = wc->audio.sample_count;

    for (int i = 0; i < 4; i++) {
        FrogTelemetryGate *gate = &wc->gates[i];
        if (gate->source == MOD_SRC_NONE) continue;
        
        float val = 0.0f;
        switch (gate->source) {
            case MOD_SRC_SUB_BASS:    val = wc->sub_bass; break;
            case MOD_SRC_MIDS:        val = wc->mids; break;
            case MOD_SRC_TREBLE:      val = wc->treble; break;
            case MOD_SRC_MASTER_PEAK: val = wc->master_peak; break;
            case MOD_SRC_MID_CENTER:  val = wc->mid_energy; break;
            case MOD_SRC_SIDE_WIDTH:  val = wc->side_energy; break;
            default:                  break;
        }
        
        bool was_open = gate->is_open;
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

float world_clock_get_modulated(const WorldClock *wc, ModDestination dest) {
    if (!wc || dest >= MOD_DEST_COUNT) return 0.0f;
    ModSource src = g_mod_matrix[dest];
    switch (src) {
        case MOD_SRC_SUB_BASS:    return wc->sub_bass;
        case MOD_SRC_MIDS:        return wc->mids;
        case MOD_SRC_TREBLE:      return wc->treble;
        case MOD_SRC_MASTER_PEAK: return wc->master_peak;
        case MOD_SRC_MID_CENTER:  return wc->mid_energy;
        case MOD_SRC_SIDE_WIDTH:  return wc->side_energy;
        default:                  return 0.0f;
    }
}
