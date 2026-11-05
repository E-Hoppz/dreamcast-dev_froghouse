//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_physics.h"
#include "frog_audio.h"
#include "frog_math.h"
#include "frog_util.h"
#include "world_clock.h"
#include "frog_air_signal.h" 
#include <dc/fmath.h>
#include <string.h>
#include <math.h>

// Global Level / Environmental Physics State (Tunable in TIMEPIECE)
FrogPhysicsEnvironment g_frog_physics_env = {
    { 0.0f, -9.8f, 0.0f, 0.0f }, // Default Gravity: -9.8 m/s^2 on Y
    1.225f,                       // Air density: 1.225 kg/m^3
    0.0f,                         // Water elevation
    1.0f,                         // Time scale: normal 1.0x speed
    1.0f                          // Room surface friction multiplier
};

// Global combat freeze-frame timer (stepped once per engine pulse)
static uint32_t s_global_hitstop = 0;

// 240Hz sub-tick accumulator (44100 / 240 = 183.75 samples per step)
#define SAMPLES_PER_TICK_240HZ 183.75f
static float s_accumulator = 0.0f;

static FrogAirDensity s_wave_smoother = {0.0f};

void frog_physics_init(void) {
    s_global_hitstop = 0;
    s_accumulator = 0.0f;
    g_frog_physics_env.time_scale = 1.0f;
    frog_util_log("PHYSICS: Universal 240Hz Multi-Genre Sandbox Initialized.");
}

void frog_physics_set_environment(vec3_t gravity, float air_density, float water_height) {
    g_frog_physics_env.gravity = gravity;
    g_frog_physics_env.air_density = air_density;
    g_frog_physics_env.water_height = water_height;
}

void frog_physics_set_time_scale(float scale) {
    g_frog_physics_env.time_scale = (scale < 0.0f) ? 0.0f : scale;
}

void frog_physics_set_hitstop(uint32_t duration_samples) {
    s_global_hitstop = duration_samples;
}

// Preset loader for TIMEPIECE level/entity inspector
void frog_physics_load_material_preset(FrogPhysicsMaterial *mat, const char *preset_name) {
    if (!mat || !preset_name) return;

    if (strcmp(preset_name, "car_racing") == 0) {
        mat->mass = 1100.0f; mat->static_friction = 1.0f; mat->dynamic_friction = 0.85f;
        mat->restitution = 0.15f; mat->aerodynamic_drag = 0.32f; mat->aerodynamic_lift = 0.0f;
        mat->buoyancy = 0.2f; mat->hydro_planing = 0.0f; mat->slope_grip = 0.6f;
        mat->max_speed = 85.0f; mat->max_air_speed = 85.0f;
    } else if (strcmp(preset_name, "surfboard") == 0) {
        mat->mass = 8.0f; mat->static_friction = 0.2f; mat->dynamic_friction = 0.05f;
        mat->restitution = 0.05f; mat->aerodynamic_drag = 0.10f; mat->aerodynamic_lift = 0.05f;
        mat->buoyancy = 2.4f; mat->hydro_planing = 1.8f; mat->slope_grip = 0.9f;
        mat->max_speed = 28.0f; mat->max_air_speed = 28.0f;
    } else if (strcmp(preset_name, "fighter_chassis") == 0) {
        mat->mass = 75.0f; mat->static_friction = 1.2f; mat->dynamic_friction = 0.95f;
        mat->restitution = 0.0f; mat->aerodynamic_drag = 0.05f; mat->aerodynamic_lift = 0.0f;
        mat->buoyancy = 0.8f; mat->hydro_planing = 0.0f; mat->slope_grip = 1.0f;
        mat->max_speed = 18.0f; mat->max_air_speed = 14.0f;
    } else if (strcmp(preset_name, "platformer_sonic") == 0) {
        mat->mass = 35.0f; mat->static_friction = 0.9f; mat->dynamic_friction = 0.75f;
        mat->restitution = 0.0f; mat->aerodynamic_drag = 0.02f; mat->aerodynamic_lift = 0.0f;
        mat->buoyancy = 1.0f; mat->hydro_planing = 0.0f; mat->slope_grip = 1.4f;
        mat->max_speed = 30.0f; mat->max_air_speed = 24.0f;
    } else if (strcmp(preset_name, "airplane_glider") == 0) {
        mat->mass = 450.0f; mat->static_friction = 0.3f; mat->dynamic_friction = 0.15f;
        mat->restitution = 0.1f; mat->aerodynamic_drag = 0.08f; mat->aerodynamic_lift = 1.6f;
        mat->buoyancy = 0.5f; mat->hydro_planing = 0.0f; mat->slope_grip = 0.2f;
        mat->max_speed = 120.0f; mat->max_air_speed = 120.0f;
    } else {
        // Universal Default
        mat->mass = 50.0f; mat->static_friction = 0.6f; mat->dynamic_friction = 0.5f;
        mat->restitution = 0.2f; mat->aerodynamic_drag = 0.15f; mat->aerodynamic_lift = 0.0f;
        mat->buoyancy = 1.0f; mat->hydro_planing = 0.0f; mat->slope_grip = 0.5f;
        mat->max_speed = 20.0f; mat->max_air_speed = 20.0f;
    }
}

void frog_physics_body_init(FrogBody *body, float mass, float radius) {
    if (!body) return;
    memset(body, 0, sizeof(FrogBody));
    body->flags = PHYS_FLAG_NONE;
    frog_physics_load_material_preset(&body->mat, "default");
    body->mat.mass = mass;

    // Default setup: 1 universal wheel
    body->wheel_count = 1;
    body->wheels[0].radius = radius > 0.01f ? radius : 0.33f;
    body->wheels[0].grip_factor = 1.0f;
    body->tagging_factor = 1.0f;
}

void frog_physics_body_apply_force(FrogBody *body, vec3_t force) {
    if (!body || (body->flags & PHYS_FLAG_STATIC)) return;
    float inv_m = (body->mat.mass > 0.001f) ? (1.0f / body->mat.mass) : 1.0f;
    body->acceleration = vec3_add(body->acceleration, vec3_scale(force, inv_m));
}

void frog_physics_body_apply_impulse(FrogBody *body, vec3_t impulse) {
    if (!body || (body->flags & PHYS_FLAG_STATIC)) return;
    float inv_m = (body->mat.mass > 0.001f) ? (1.0f / body->mat.mass) : 1.0f;
    body->velocity = vec3_add(body->velocity, vec3_scale(impulse, inv_m));
}

// CS 1.6 Tagging / Inelastic collision clamp
void frog_physics_body_tag(FrogBody *body, float velocity_scrub_percent) {
    if (!body) return;
    float factor = 1.0f - velocity_scrub_percent;
    if (factor < 0.1f) factor = 0.1f;
    body->velocity.x *= factor;
    body->velocity.z *= factor;
}

// --- [ RIGID 240HZ SEMI-IMPLICIT EULER STEP ] ---
// Pure, deterministic, linear math. Frame-rate independent!
void frog_physics_step_fixed_240hz(FrogBody *body, float dt) {
    if (!body || (body->flags & PHYS_FLAG_STATIC)) return;

    FrogPhysicsMaterial *mat = &body->mat;

    // Apply Hitstun (Fighters / Combat)
    if (body->hitstun_samples > 0) {
        uint32_t samples_in_step = (uint32_t)(dt * 44100.0f);
        if (body->hitstun_samples > samples_in_step) {
            body->hitstun_samples -= samples_in_step;
            return;
        } else {
            body->hitstun_samples = 0;
        }
    }

    // 1. Gravity Vector (Independent of body mass, respects environment vector)
    if (!(body->flags & PHYS_FLAG_NO_GRAVITY) && !(body->flags & PHYS_FLAG_GROUNDED)) {
        body->velocity = vec3_add(body->velocity, vec3_scale(g_frog_physics_env.gravity, dt));
    }

    // 2. Linear Acceleration integration: v += a * dt
    body->velocity.x += body->acceleration.x * dt;
    body->velocity.y += body->acceleration.y * dt;
    body->velocity.z += body->acceleration.z * dt;

    // 3. Dynamic Friction & Drag Damping
    float ground_fric = (body->flags & PHYS_FLAG_GROUNDED) ? (mat->dynamic_friction * g_frog_physics_env.surface_friction) : 0.0f;
    float total_damping = 1.0f - ((ground_fric + (mat->aerodynamic_drag * g_frog_physics_env.air_density)) * dt);
    if (total_damping < 0.0f) total_damping = 0.0f;

    body->velocity.x *= total_damping;
    body->velocity.z *= total_damping;

    // 4. Maximum Velocity Clamping
    float horiz_speed = sqrtf((body->velocity.x * body->velocity.x) + (body->velocity.z * body->velocity.z));
    float cap = (body->flags & PHYS_FLAG_GROUNDED) ? mat->max_speed : mat->max_air_speed;
    if (cap > 0.0f && horiz_speed > cap) {
        float scale = cap / horiz_speed;
        body->velocity.x *= scale;
        body->velocity.z *= scale;
    }

    // 5. Plane Constraints (2D Fighters, Sidescrollers, Top-Down RPGs)
    if (body->flags & PHYS_FLAG_LOCK_Z) body->position.z = 0.0f;
    if (body->flags & PHYS_FLAG_LOCK_Y) body->position.y = 0.0f;

    // 6. Position Integration: p += v * dt
    body->prev_position = body->position;
    body->position.x += body->velocity.x * dt;
    body->position.y += body->velocity.y * dt;
    body->position.z += body->velocity.z * dt;

    // Reset single-frame acceleration
    body->acceleration = (vec3_t){0, 0, 0, 0};

    // Advance all active wheel phases
    for (uint32_t i = 0; i < body->wheel_count; i++) {
        frog_wheel_advance(&body->wheels[i], dt);
    }
}

// Master entry point: Dispatches 240Hz sub-ticks slaved to the AICA sample clock
void frog_physics_update(FrogBody *body, uint32_t delta_samples) {
    if (!body) return;

    if (s_global_hitstop > 0) {
        if (delta_samples >= s_global_hitstop) {
            delta_samples -= s_global_hitstop;
            s_global_hitstop = 0;
        } else {
            s_global_hitstop -= delta_samples;
            return;
        }
    }

    // Scale delta samples by global level time dilation (bullet time)
    float effective_samples = (float)delta_samples * g_frog_physics_env.time_scale;
    s_accumulator += effective_samples;

    const float fixed_dt = 1.0f / 240.0f;
    int max_substeps = 8; // Clamps debug pauses

    while (s_accumulator >= SAMPLES_PER_TICK_240HZ && max_substeps > 0) {
        frog_physics_step_fixed_240hz(body, fixed_dt);
        s_accumulator -= SAMPLES_PER_TICK_240HZ;
        max_substeps--;
    }

    if (max_substeps == 0) {
        s_accumulator = 0.0f;
    }
}

// --- [ MOVEMENT ACTUATORS ] ---

// Multi-Wheel Drivetrain (Skateboard, Car, Tank, Truck)
void frog_physics_update_wheels(FrogBody *body, float throttle, float steering, float brake, float clutch) {
    if (!body || body->wheel_count == 0) return;

    float forward_speed = (body->velocity.x * cosf(body->yaw)) + (body->velocity.z * sinf(body->yaw));

    for (uint32_t i = 0; i < body->wheel_count; i++) {
        FrogWheel *w = &body->wheels[i];

        // Apply steering to front wheels (indices 0 and 1)
        if (i < 2) w->steer_angle = steering;

        if (w->is_powered) {
            float target_rate = (throttle * (body->mat.max_speed / (w->radius * 6.28318f))) * (1.0f - clutch);
            w->rate = target_rate;
        }

        // Contact patch velocity = rate * 2 * PI * radius
        float patch_speed = (float)w->rate * (6.28318f * w->radius);
        float speed_floor = fmaxf(fabsf(forward_speed), 1.0f);
        w->slip_ratio = (patch_speed - forward_speed) / speed_floor;

        // Dynamic tire friction curve
        float abs_slip = fabsf(w->slip_ratio);
        if (abs_slip < 0.20f) {
            w->grip_factor = 1.0f;
        } else if (abs_slip < 0.60f) {
            w->grip_factor = 1.0f - ((abs_slip - 0.20f) * 1.5f);
        } else {
            w->grip_factor = 0.40f; // Kinetic slide (drift)
        }

        // Drive force
        float force_magnitude = (patch_speed - forward_speed) * body->mat.dynamic_friction * w->grip_factor * (1.0f - brake);
        vec3_t drive_force = {
            force_magnitude * cosf(body->yaw + w->steer_angle),
            0.0f,
            force_magnitude * sinf(body->yaw + w->steer_angle),
            0.0f
        };
        frog_physics_body_apply_force(body, drive_force);
    }
}

// Hydrodynamics (Surfing, Boats, Jetskis)
void frog_physics_update_hydrodynamics(FrogBody *body, WorldClock *wc) {
    if (!body || !wc || !(body->flags & PHYS_FLAG_WATER_INTERACTION)) return;

    float wave_y = frog_physics_get_wave_height(body->position.x, body->position.z, wc);
    float water_surface = g_frog_physics_env.water_height + wave_y;
    float depth = water_surface - body->position.y;

    if (depth > 0.0f) {
        // Planing Lift: Speed pushes the surfboard/jetski hull up out of the water!
        float horiz_speed = sqrtf((body->velocity.x * body->velocity.x) + (body->velocity.z * body->velocity.z));
        float planing_lift = horiz_speed * body->mat.hydro_planing * 0.5f;

        // Archimedes Buoyancy Force = depth * buoyancy * gravity
        float buoyant_acc = (depth * body->mat.buoyancy * 9.8f) + planing_lift;
        body->velocity.y += buoyant_acc * wc->delta_time;

        // Planing cuts submerged drag down by up to 70%!
        float drag_reduction = 1.0f / (1.0f + (planing_lift * 0.2f));
        float water_drag = body->mat.dynamic_friction * 1.5f * drag_reduction * wc->delta_time;
        float factor = 1.0f - water_drag;
        if (factor < 0.0f) factor = 0.0f;

        body->velocity.x *= factor;
        body->velocity.z *= factor;
    }
}

// Aerodynamics (Planes, Gliders, Parachutes, Air Dives)
void frog_physics_update_aerodynamics(FrogBody *body) {
    if (!body || !(body->flags & PHYS_FLAG_AERODYNAMICS)) return;

    float airspeed_sq = (body->velocity.x * body->velocity.x) + 
                        (body->velocity.y * body->velocity.y) + 
                        (body->velocity.z * body->velocity.z);

    if (airspeed_sq < 0.1f) return;

    // Wing Lift = 0.5 * density * v^2 * Area * LiftCoeff
    float lift_force = 0.5f * g_frog_physics_env.air_density * airspeed_sq * body->mat.aerodynamic_lift * 0.05f;
    body->acceleration.y += lift_force / body->mat.mass;
}

// Combat Knockback & Hitstun (2D/3D Fighters & Shooters)
void frog_physics_fighting_apply_knockback(FrogBody *attacker, FrogBody *victim, vec3_t force, uint32_t hitstop_samples, WorldClock *wc) {
    (void)attacker;
    if (!victim || !wc) return;

    // Freeze both entities in hitstop
    frog_physics_set_hitstop(hitstop_samples);
    victim->hitstun_samples = hitstop_samples * 2;

    // Apply knockback impulse inversely proportional to victim's weight
    frog_physics_body_apply_impulse(victim, force);

    // Trigger rumble & vibration telemetry
    frog_audio_trigger_momentum(GHOST_CH_VIBRATION, 8);
}

// Platformer variable jump (cutting button early drops jump apex)
void frog_physics_platformer_jump(FrogBody *body, float jump_velocity, bool variable_cut) {
    if (!body) return;
    if (!variable_cut) {
        body->velocity.y = jump_velocity;
        body->flags &= ~PHYS_FLAG_GROUNDED;
    } else {
        // Cutting jump button short drops vertical speed by 50%
        if (body->velocity.y > 0.0f) {
            body->velocity.y *= 0.5f;
        }
    }
}

float frog_physics_get_wave_height(float x, float z, WorldClock *wc) {
    if (!wc) return 0.0f;
    float phase_a = (float)(wc->audio.sample_count % 1024) / 1024.0f;
    float phase_b = (float)((wc->audio.sample_count / 2) % 1024) / 1024.0f;

    float swell = fsin(phase_a * 6.28318f + (x * 0.05f)) * 2.0f;
    float chop  = fcos(phase_b * 6.28318f + (z * 0.1f)) * 0.5f;

    return frog_air_step_density(&s_wave_smoother, swell + chop);
}

bool frog_physics_rhythm_check_hit(uint32_t target_sample, uint32_t tolerance, WorldClock *wc) {
    if (!wc) return false;
    uint32_t current = wc->audio.sample_count;
    uint32_t delta = (current >= target_sample) ? (current - target_sample) : (target_sample - current);
    return (delta <= tolerance);
}

void frog_physics_rhythm_scrub(float stick_deflection, float base_pitch, WorldClock *wc) {
    if (!wc) return;
    float target = base_pitch + (stick_deflection * 1.5f);
    target = frog_clamp(target, 0.05f, 2.5f);
    frog_audio_set_pitch(0, target);
}
