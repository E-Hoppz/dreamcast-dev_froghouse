#ifndef FROG_CONFIG_H
#define FROG_CONFIG_H

#include <arch/types.h>
#include <stdint.h>  
#include <stdbool.h>

// --- [ DISPLAY SETTINGS ] ---
#define SCREEN_WIDTH       640
#define SCREEN_HEIGHT      480
#define DEFAULT_BG_COLOR   0xFF000000 // Lair Black

// --- [ PHYSICS & TIMING ] ---
#define TARGET_FPS         60
#define FIXED_DELTA_TIME   (1.0f / (float)TARGET_FPS)
#define MAX_PHYSICS_STEPS  5

// --- [ AUDIO SYNC (RHYTHM GAME) ] ---
#define AUDIO_BUFFER_SIZE  16384
#define RHYTHM_LATENCY_MS  15.0f  

// --- [ PERSPECTIVE (RACING GAME) ] ---
#define CAMERA_FOV         60.0f
#define CAMERA_NEAR        0.1f
#define CAMERA_FAR         1000.0f

// --- [ THE HARDWARE CONTRACT RAILS ] ---
typedef enum {
    CONTRACT_OPAQUE,     // First-Party high-fidelity Dreamcast rendering
    CONTRACT_GLIDE,      // 3dfx Warm Bilinear soft-clip and retro color banding
    CONTRACT_MIPS,       // PS1 Rigid Integer coordinate snapping (jittery polygons)
    CONTRACT_PARADOX     // SPU-Locked Audio-Driven Vertex Chaos (transient vertex explosion)
} FrogContract;

// --- [ GLOBAL STATE ] ---
typedef struct {
    bool debug_mode;
    bool vsync_enabled;
    float global_volume;
    uint32_t frame_count;
    FrogContract active_contract; // The active hardware rendering contract
} FrogConfig;

extern FrogConfig frog_engine_config;

void frog_config_init(void);

#endif
