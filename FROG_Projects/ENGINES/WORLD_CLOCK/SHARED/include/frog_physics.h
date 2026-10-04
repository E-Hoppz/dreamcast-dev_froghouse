#ifndef FROG_PHYSICS_H
#define FROG_PHYSICS_H

#include <stdint.h>
#include "frog_math_3d.h"
#include "world_clock.h" 

// --- [ THE AICA REGISTER MAP ] ---
#define GHOST_CH_WAVE_ALPHA   60  
#define GHOST_CH_WAVE_BETA    61  
#define GHOST_CH_VIBRATION    62  
#define GHOST_CH_RHYTHM_SYNC  63  

// --- [ WORLD CLOCK DYNAMICS ] ---

typedef struct {
    float mass;
    float drag;         
    float friction;     
    float elasticity;   
    float buoyancy;     
} FrogPhysicsConstants;

// [REPAIRMAN'S SHUNT] Strict 64-Byte Structure Striding (Pads structure to exactly 64 bytes)
// This aligns the structure with two 32-byte cache lines, preventing SH-4 cache thrashing during loops.
// Struct renamed to FrogPhysParticle to prevent compilation conflicts with visual frog_particle.h.
typedef struct {
    vec3_t position;      // 16 bytes
    vec3_t prev_position; // 16 bytes
    vec3_t velocity;      // 16 bytes
    vec3_t acceleration;  // 16 bytes
} __attribute__((aligned(32))) FrogPhysParticle;

typedef struct {
    vec3_t position;     
    vec3_t prev_position; 
    vec3_t velocity;     
    vec3_t acceleration; 
    float  heading;      
    float  padding[3];    // 12 bytes - Pads struct to exactly 64 bytes (Two cache lines)
} __attribute__((aligned(32))) FrogBody;

typedef struct {
    float amplitude; 
    float frequency; 
    uint32_t ghost_channel; 
} FrogOscillator;

typedef struct {
    uint32_t body_idx_a;
    uint32_t body_idx_b;
    float    rest_length;
    float    stiffness;
} FrogPBDDistanceConstraint;

// --- [ THE CORE LOGIC GATES ] ---
void frog_physics_init();
void frog_physics_update(FrogBody *body, FrogPhysicsConstants *cfg, uint32_t delta_samples);
void frog_physics_apply_impulse(FrogBody *body, vec3_t force);

// --- [ NEW VERLET & POSITION-BASED DYNAMICS ] ---
void frog_physics_verlet_update(FrogBody *body, FrogPhysicsConstants *cfg, uint32_t delta_samples);
void frog_physics_pbd_resolve_distance(FrogBody *b1, FrogBody *b2, float rest_length, float stiffness);

// --- [ COMMON HARDWARE MATH REEFS ] ---
float frog_physics_get_wave_height(float x, float z, WorldClock *wc);
float frog_physics_get_resonance_jitter(WorldClock *wc);
void  frog_physics_apply_envelope_momentum(FrogBody *body, uint32_t ghost_ch);

// --- [ GENRE-SPECIFIC DUAL-OSCILLATOR HOOKS ] ---
// Keep these completely intact. You can now call and mix-and-match these inside 
// your game's modular entity callbacks (e.g. calling surfing and racing together).
void frog_physics_racing_update(FrogBody *body, float throttle, float clutch, 
                                 int gear, float slope_incline, 
                                 float *out_slip, WorldClock *wc);

bool frog_physics_rhythm_check_hit(uint32_t target_sample, uint32_t tolerance, WorldClock *wc);
void frog_physics_rhythm_scrub(float stick_deflection, float base_pitch, WorldClock *wc);

void frog_physics_fighting_apply_impact(FrogBody *body, float force, 
                                         uint32_t freeze_duration_samples, WorldClock *wc);

void frog_physics_platformer_jump(FrogBody *body, float jump_strength, float mass);
void frog_physics_platformer_land(FrogBody *body, float landing_force, FrogPhysicsConstants *cfg);

void frog_physics_surfing_update(FrogBody *body, FrogPhysicsConstants *cfg, WorldClock *wc);

#endif
