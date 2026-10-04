#include "timepiece_clock.h"
#include <cmath>
#include <iostream>

struct AudioDeck g_audio_deck = { nullptr, 0, 0, false, 0.0f, 120.0f };

extern "C" {

        extern WorldClock wc;
    // Map global routing matrix locally for editor
    ModSource g_mod_matrix[MOD_DEST_COUNT] = {
        MOD_SRC_SUB_BASS,    // MOD_DEST_LIGHT_GAIN
        MOD_SRC_TREBLE,      // MOD_DEST_STROKE_JITTER
        MOD_SRC_SUB_BASS,    // MOD_DEST_VMU_BAR
        MOD_SRC_SUB_BASS     // MOD_DEST_MESH_DISPLACEMENT
    };

    // Implements simulated bare-metal initializers
    void world_clock_init(WorldClock *wc) {
        wc->audio.sample_count = 0;
        wc->audio.monotonic_samples = 0;
        wc->audio.current_offset = 0;
        wc->delta_time = 0.0f;
        
        wc->sub_bass = 0.0f;
        wc->mids = 0.0f;
        wc->treble = 0.0f;
        wc->master_peak = 0.0f;
        wc->pan_bias = 0.0f; 
        wc->mid_energy = 0.0f;
        wc->side_energy = 0.0f;

        for (int i = 0; i < 4; i++) {
            wc->gates[i].source = MOD_SRC_NONE;
            wc->gates[i].threshold = 0.0f;
            wc->gates[i].hysteresis = 0.0f;
            wc->gates[i].is_open = false;
            wc->gates[i].on_trigger = NULL;
        }
    }

    // Handles temporal updates and hysteresis checks
    void world_clock_update(WorldClock *wc) {
        uint32_t delta = g_audio_deck.is_playing ? 735 : 0; // Simulated ~16.6ms frame delta
        wc->audio.sample_count = (wc->audio.sample_count + delta) % 65536;
        wc->audio.monotonic_samples += delta;
        wc->delta_time = (float)delta / 44100.0f;

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
}

void LoadWAVMusic(const char* filepath) {
    if (g_audio_deck.device_id != 0) {
        SDL_CloseAudioDevice(g_audio_deck.device_id);
        SDL_FreeWAV(g_audio_deck.wav_buffer);
        g_audio_deck.device_id = 0;
    }

    SDL_AudioSpec wav_spec;
    if (SDL_LoadWAV(filepath, &wav_spec, &g_audio_deck.wav_buffer, &g_audio_deck.wav_length) == NULL) {
        std::cerr << "WAV Load Error: " << SDL_GetError() << std::endl;
        return;
    }

    g_audio_deck.device_id = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
    g_audio_deck.is_playing = false;
    g_audio_deck.current_time = 0.0f;

    float bytes_per_sec = wav_spec.channels * wav_spec.freq * (SDL_AUDIO_BITSIZE(wav_spec.format) / 8);
    g_audio_deck.duration = (float)g_audio_deck.wav_length / bytes_per_sec;
}

void SimulateAudioTelemetry(float dt) {
    if (g_audio_deck.is_playing) {
        g_audio_deck.current_time += dt;
        if (g_audio_deck.current_time >= g_audio_deck.duration) {
            g_audio_deck.current_time = 0.0f;
        }
        
        float t = g_audio_deck.current_time;
        wc.sub_bass    = fabsf(sinf(t * 3.14f) * 0.7f + cosf(t * 1.5f) * 0.3f);
        wc.mids        = fabsf(cosf(t * 5.23f) * 0.5f + sinf(t * 2.1f) * 0.5f);
        wc.treble      = fabsf(sinf(t * 12.0f) * 0.4f);
        wc.master_peak = fmaxf(wc.sub_bass, fmaxf(wc.mids, wc.treble));

        wc.mid_energy  = wc.mids * 0.9f;
        wc.side_energy = wc.treble * 0.6f;
        wc.pan_bias    = sinf(t * 0.5f) * 0.5f; 
    } else {
        wc.sub_bass = 0.0f;
        wc.mids = 0.0f;
        wc.treble = 0.0f;
        wc.master_peak = 0.0f;
        wc.mid_energy = 0.0f;
        wc.side_energy = 0.0f;
        wc.pan_bias = 0.0f;
    }
}
