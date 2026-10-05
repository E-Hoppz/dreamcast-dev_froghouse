#ifndef WORLD_CLOCK_H
#define WORLD_CLOCK_H

#include "frog_audio.h" 
#include "frog_input.h"
#include "frog_draw.h"

struct WorldClock;

// --- [ THE MODULATION MATRIX ENUMS ] ---
typedef enum {
    MOD_SRC_SUB_BASS,
    MOD_SRC_MIDS,
    MOD_SRC_TREBLE,
    MOD_SRC_MASTER_PEAK,
    MOD_SRC_MID_CENTER,   // SPU Mid-channel (mono center) energy
    MOD_SRC_SIDE_WIDTH,   // SPU Side-channel (stereo width) energy
    MOD_SRC_NONE
} ModSource;

typedef enum {
    MOD_DEST_LIGHT_GAIN,        // Driven by frog_color.c light map overdrive
    MOD_DEST_STROKE_JITTER,      // Driven by frog_color.c canvas stroke jitter
    MOD_DEST_VMU_BAR,           // Driven by frog_vmu_visuals.c LCD bar height
    MOD_DEST_MESH_DISPLACEMENT,   // Driven by frog_mesh.c vertex explosion
    MOD_DEST_COUNT
} ModDestination;

// Callback signature for our active signal router
typedef void (*FrogGateCallback)(struct WorldClock *wc, bool is_open);

typedef struct {
    ModSource source;     // Mapped input source (e.g., MOD_SRC_SUB_BASS)
    float     threshold;  // Activation threshold (0.0 to 1.0)
    float     hysteresis; // Safety margin to prevent rapid ON/OFF flutter
    bool      is_open;    // Current state (true = open, false = closed)
    FrogGateCallback on_trigger; // Callback triggered on gate state changes
} FrogTelemetryGate;

/**
 * [THE MASTER MOTHERBOARD]
 */
typedef struct WorldClock {
    // --- [ CORE TIMING RAIL ] ---
    AudioClock audio;       
    float      delta_time;  

    // --- [ INPUT SIGNAL BUS ] ---
    FrogInput  input;       

    // --- [ THE FREQUENCY TELEMETRY ] ---
    float sub_bass;    
    float mids;        
    float treble;      
    float master_peak; 
    
    // --- [ SPATIAL PANNING TELEMETRY ] ---
    float pan_bias; 
    float mid_energy;   
    float side_energy;  

    // --- [ THE SPU CACHED PLAYHEAD REGISTERS ] ---
    uint32_t engine_playhead_cache; // Cached alpha channel offset
    uint32_t wheel_playhead_cache;  // Cached beta channel offset

    // --- [ THE SPU MAILBOX TELEMETRY GATES ] ---
    FrogTelemetryGate gates[4]; // 4 independent configurable hardware gates
} WorldClock;

// Global routing matrix
extern ModSource g_mod_matrix[MOD_DEST_COUNT];

void world_clock_init(WorldClock *wc);
void world_clock_update(WorldClock *wc);
void world_clock_configure_gate(WorldClock *wc, int gate_idx, ModSource source, float threshold, float hysteresis, FrogGateCallback callback);

static inline float world_clock_get_modulated(const WorldClock *wc, ModDestination dest) {
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

#endif
