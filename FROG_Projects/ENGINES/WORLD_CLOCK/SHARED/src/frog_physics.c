#include "frog_physics.h"
#include "frog_audio.h"
#include "frog_math.h"
#include "frog_util.h"
#include "world_clock.h"
#include "frog_air_signal.h" 
#include <dc/fmath.h> // Hardware FSCA lookups
#include <math.h>

static FrogAirPressure gravity_rail = {0.0f, 0.5f, 1.0f};
static FrogAirDensity  swell_smoother = {0.0f}; 

static const float gear_ratios[6] = { 0.0f, 3.2f, 2.1f, 1.5f, 1.2f, 0.9f };

static uint32_t hit_stop_timer = 0;

void frog_physics_init() {
    frog_util_log("PHYSICS: Multi-Genre Audio-Clocked Engine Online.");
}

void frog_physics_apply_impulse(FrogBody *body, vec3_t force) {
    body->acceleration = vec3_add(body->acceleration, force);
}

void frog_physics_update(FrogBody *body, FrogPhysicsConstants *cfg, uint32_t delta_samples) {
    if (hit_stop_timer > 0) {
        if (delta_samples >= hit_stop_timer) {
            hit_stop_timer = 0;
        } else {
            hit_stop_timer -= delta_samples;
            return; 
        }
    }

    float dt = (float)delta_samples / 44100.0f;
    if (dt <= 0.0f || dt > 0.1f) return; 

    float raw_gravity = 9.8f * dt;
    float heavy_gravity = frog_air_step_pressure(&gravity_rail, raw_gravity);
    body->velocity.y -= (heavy_gravity * cfg->mass);

    body->velocity.x += frog_air_step_spiral(body->acceleration.x * dt);
    body->velocity.y += body->acceleration.y * dt; 
    body->velocity.z += frog_air_step_spiral(body->acceleration.z * dt);

    float friction_factor = 1.0f - (cfg->friction * dt);
    body->velocity.x *= friction_factor;
    body->velocity.z *= friction_factor;

    body->position.x += body->velocity.x * dt;
    body->position.y += body->velocity.y * dt;
    body->position.z += body->velocity.z * dt;

    body->acceleration = (vec3_t){0, 0, 0, 0};
}

void frog_physics_verlet_update(FrogBody *body, FrogPhysicsConstants *cfg, uint32_t delta_samples) {
    if (!body || !cfg) return;
    
    if (hit_stop_timer > 0) {
        if (delta_samples >= hit_stop_timer) {
            hit_stop_timer = 0;
        } else {
            hit_stop_timer -= delta_samples;
            return; 
        }
    }

    float dt = (float)delta_samples / 44100.0f;
    if (dt <= 0.0f || dt > 0.1f) return;

    vec3_t temp_pos = body->position;

    float friction_factor = 1.0f - (cfg->friction * dt);
    vec3_t displacement = vec3_scale(vec3_sub(body->position, body->prev_position), friction_factor);

    float raw_gravity = 9.8f * dt;
    float heavy_gravity = frog_air_step_pressure(&gravity_rail, raw_gravity);
    vec3_t acc_gravity = {0.0f, -(heavy_gravity * cfg->mass) / dt, 0.0f, 0.0f};

    vec3_t total_acc = vec3_add(body->acceleration, acc_gravity);
    vec3_t term_acc = vec3_scale(total_acc, dt * dt);

    body->position = vec3_add(body->position, vec3_add(displacement, term_acc));
    body->prev_position = temp_pos;

    body->velocity = vec3_scale(vec3_sub(body->position, body->prev_position), 1.0f / dt);
    body->acceleration = (vec3_t){0, 0, 0, 0};
}

void frog_physics_pbd_resolve_distance(FrogBody *b1, FrogBody *b2, float rest_length, float stiffness) {
    if (!b1 || !b2) return;

    vec3_t delta = vec3_sub(b1->position, b2->position);
    float len = vec3_length(delta);

    if (len < 0.0001f) return; 

    float diff = rest_length - len;
    
    float percent = (diff / len) * stiffness * 0.5f;
    vec3_t offset = vec3_scale(delta, percent);

    b1->position = vec3_add(b1->position, offset);
    b2->position = vec3_sub(b2->position, offset);
}

float frog_physics_get_wave_height(float x, float z, WorldClock *wc) {
    float phase_a = (float)(wc->audio.sample_count % 1024) / 1024.0f;
    float phase_b = (float)((wc->audio.sample_count / 2) % 1024) / 1024.0f;

    float swell = fsin(phase_a * 6.28318f + (x * 0.05f)) * 2.0f;
    float chop  = fcos(phase_b * 6.28318f + (z * 0.1f)) * 0.5f;

    return frog_air_step_density(&swell_smoother, swell + chop);
}

float frog_physics_get_resonance_jitter(WorldClock *wc) {
    return (float)(wc->audio.sample_count % 100) / 100.0f;
}

void frog_physics_apply_envelope_momentum(FrogBody *body, uint32_t ghost_ch) {
    uint32_t vol = frog_audio_get_envelope_vol(ghost_ch);
    float multiplier = (float)vol / 255.0f;
    
    multiplier = frog_air_step_spiral(multiplier);
    
    body->velocity.x *= multiplier;
    body->velocity.z *= multiplier;
}

// ================================================================
// GENRE SPECIFIC HARDWARE HOOKS
// ================================================================

void frog_physics_racing_update(FrogBody *body, float throttle, float clutch, 
                                 int gear, float slope_incline, 
                                 float *out_slip, WorldClock *wc) {
    if (!body || !wc) return;

    float slope_factor = 1.0f - (slope_incline * 0.15f);

    float target_engine_pitch = (1.0f + (throttle * 3.0f)) * slope_factor;
    frog_audio_set_pitch(GHOST_CH_WAVE_ALPHA, target_engine_pitch);

    float ratio = gear_ratios[gear];
    float target_wheel_pitch = target_engine_pitch * ratio * (1.0f - clutch);
    frog_audio_set_pitch(GHOST_CH_WAVE_BETA, target_wheel_pitch);

    uint32_t engine_playhead = wc->engine_playhead_cache;
    uint32_t wheel_playhead = wc->wheel_playhead_cache;
    
    float phase_shift = fabsf((float)engine_playhead - (float)wheel_playhead);
    *out_slip = frog_clamp(phase_shift / 16384.0f, 0.0f, 1.0f);

    float dt = wc->delta_time;
    float engine_force = target_wheel_pitch * 50.0f;
    float drag_force = vec3_length(body->velocity) * body->velocity.x * 0.05f;

    body->velocity.x += (engine_force - drag_force) * dt;
}

bool frog_physics_rhythm_check_hit(uint32_t target_sample, uint32_t tolerance, WorldClock *wc) {
    if (!wc) return false;
    uint32_t current_playhead = wc->audio.sample_count;
    uint32_t delta = (current_playhead >= target_sample) ? 
                     (current_playhead - target_sample) : (target_sample - current_playhead);

    return (delta <= tolerance);
}

void frog_physics_rhythm_scrub(float stick_deflection, float base_pitch, WorldClock *wc) {
    if (!wc) return;
    float target_pitch = base_pitch + (stick_deflection * 1.5f);
    if (target_pitch < -2.0f) target_pitch = -2.0f;
    if (target_pitch > 2.0f)  target_pitch = 2.0f;

    frog_audio_set_pitch(0, target_pitch);
}

void frog_physics_fighting_apply_impact(FrogBody *body, float force, 
                                         uint32_t freeze_duration_samples, WorldClock *wc) {
    if (!body || !wc) return;

    frog_audio_trigger_momentum(GHOST_CH_VIBRATION, 8);
    hit_stop_timer = freeze_duration_samples;

    vec3_t impact_dir = { -body->velocity.x * 0.5f, force * 2.0f, 0.0f, 0.0f };
    body->velocity = vec3_add(body->velocity, impact_dir);
}

void frog_physics_platformer_jump(FrogBody *body, float jump_strength, float mass) {
    if (!body) return;
    float spiral_force = frog_air_step_spiral(jump_strength / mass);
    body->velocity.y = spiral_force;
}

void frog_physics_platformer_land(FrogBody *body, float landing_force, FrogPhysicsConstants *cfg) {
    if (!body || !cfg) return;
    FrogAirPressure landing_pressure = { 0.0f, cfg->friction, landing_force };
    float heavy_landing = frog_air_step_pressure(&landing_pressure, landing_force);
    body->velocity.x *= (1.0f - heavy_landing);
}

void frog_physics_surfing_update(FrogBody *body, FrogPhysicsConstants *cfg, WorldClock *wc) {
    if (!body || !cfg || !wc) return;

    float water_y = frog_physics_get_wave_height(body->position.x, body->position.z, wc);
    float submergence = water_y - body->position.y;

    if (submergence > 0.0f) {
        float upward_thrust = submergence * cfg->buoyancy;
        body->velocity.y += upward_thrust * wc->delta_time;

        float fluid_drag = 1.0f - (cfg->drag * 2.0f * wc->delta_time);
        body->velocity.x *= fluid_drag;
        body->velocity.z *= fluid_drag;
    }
}
