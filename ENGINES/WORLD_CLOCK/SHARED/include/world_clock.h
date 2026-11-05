#ifndef WORLD_CLOCK_H
#define WORLD_CLOCK_H

#include "frog_audio.h" 
#include "frog_input.h"
#include "frog_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

struct WorldClock;

typedef enum {
    MOD_SRC_SUB_BASS,
    MOD_SRC_MIDS,
    MOD_SRC_TREBLE,
    MOD_SRC_MASTER_PEAK,
    MOD_SRC_MID_CENTER,
    MOD_SRC_SIDE_WIDTH,
    MOD_SRC_NONE
} ModSource;

typedef enum {
    MOD_DEST_LIGHT_GAIN,
    MOD_DEST_STROKE_JITTER,
    MOD_DEST_VMU_BAR,
    MOD_DEST_MESH_DISPLACEMENT,
    MOD_DEST_COUNT
} ModDestination;

typedef void (*FrogGateCallback)(struct WorldClock *wc, bool is_open);

typedef struct {
    ModSource source;
    float     threshold;
    float     hysteresis;
    bool      is_open;
    FrogGateCallback on_trigger;
} FrogTelemetryGate;

typedef struct WorldClock {
    AudioClock audio;       
    float      delta_time;  
    FrogInput  input;       

    float sub_bass;    
    float mids;        
    float treble;      
    float master_peak; 
    
    float pan_bias; 
    float mid_energy;   
    float side_energy;  

    uint32_t engine_playhead_cache;
    uint32_t wheel_playhead_cache;

    FrogTelemetryGate gates[4];
} WorldClock;

extern ModSource g_mod_matrix[MOD_DEST_COUNT];

void world_clock_init(WorldClock *wc);
void world_clock_update(WorldClock *wc);
void world_clock_configure_gate(WorldClock *wc, int gate_idx, ModSource source, float threshold, float hysteresis, FrogGateCallback callback);
float world_clock_get_modulated(const WorldClock *wc, ModDestination dest);

#ifdef __cplusplus
}
#endif

#endif
