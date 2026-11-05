#ifndef FROG_PHYSICS_H
#define FROG_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>
#include "frog_math_3d.h"

#ifdef __cplusplus
extern "C" {
#endif

struct WorldClock;

// --- [ PHYSICAL BEHAVIOR FLAGS (BITMASK) ] ---
#define PHYS_FLAG_NONE             0x0000
#define PHYS_FLAG_STATIC           0x0001 // Immovable terrain, walls, brushes
#define PHYS_FLAG_NO_GRAVITY       0x0002 // Shmups, floating pickups, space craft
#define PHYS_FLAG_LOCK_Z           0x0004 // 2D Sidescroller / 2D Fighter plane lock
#define PHYS_FLAG_LOCK_Y           0x0008 // Top-Down 2D RPG plane lock
#define PHYS_FLAG_WATER_INTERACTION 0x0010 // Samples dynamic wave heights & buoyancy
#define PHYS_FLAG_AERODYNAMICS     0x0020 // Calculates aerodynamic wing lift and pitch/yaw glide
#define PHYS_FLAG_GROUNDED         0x0040 // Body is actively touching surface

// Maximum wheel actuators per chassis (Skateboard=2, Car=4, Tank=6)
#define FROG_MAX_WHEELS 6

// --- [ THE FROGWHEEL: THE AUDIO/PHYSICAL ROTATIONAL PRIMITIVE ] ---
typedef struct {
    double turns;          // Total revolutions; monotonic, never wraps
    double rate;           // Revolutions per second (Hz)
    float  radius;         // Wheel radius in meters
    float  slip_ratio;     // Longitudinal slip
    float  lateral_slip;   // Lateral slip angle
    float  grip_factor;    // Current dynamic tire friction (0.0 to 1.0)
    float  drive_torque;   // Motor force applied to wheel
    float  brake_torque;   // Braking force applied to wheel
    float  steer_angle;    // Steering deflection in radians
    bool   is_powered;     // True if receiving drivetrain power
} FrogWheel;

static inline void frog_wheel_advance(FrogWheel *w, double dt) {
    if (w) w->turns += w->rate * dt;
}

// --- [ MODULAR PHYSICAL MATERIAL / PER-OBJECT PROPERTIES ] ---
// Exposed directly to TIMEPIECE for live parameter tuning!
typedef struct {
    float mass;             // Mass in kg (affects momentum transfer, not gravity acceleration)
    float static_friction;  // Breakaway friction coefficient
    float dynamic_friction; // Sliding friction coefficient
    float restitution;      // Bounciness (0.0 = inelastic tagging, 1.0 = superball)
    float aerodynamic_drag; // Air resistance coefficient
    float aerodynamic_lift; // Wing / Glider lift multiplier
    float buoyancy;         // Water displacement (<1 sinks, >1 floats, 1 neutral)
    float hydro_planing;    // Planing lift factor: high speed lifts hull out of water!
    float slope_grip;       // Surface stickiness on steep inclines (Sonic loop adhesion)
    float max_speed;        // Maximum terminal ground speed clamp
    float max_air_speed;    // Maximum air velocity clamp
} FrogPhysicsMaterial;

// --- [ LEVEL / ROOM GLOBAL ENVIRONMENT ] ---
typedef struct {
    vec3_t gravity;         // 3D vector: standard is (0, -9.8, 0). Can be sideways or zero-g!
    float  air_density;     // Atmospheric density (modulates drag & lift)
    float  water_height;    // Base water surface plane elevation
    float  time_scale;      // Global bullet-time / slow-motion dilation (default 1.0)
    float  surface_friction;// Global ground grip modifier for current room (e.g. ice level)
} FrogPhysicsEnvironment;

// --- [ THE UNIVERSAL RIGID BODY ] ---
typedef struct FrogBody {
    vec3_t position;
    vec3_t prev_position;
    vec3_t velocity;
    vec3_t acceleration;
    
    // Euler Orientation (radians)
    float  yaw;
    float  pitch;
    float  roll;

    uint32_t flags;
    
    FrogPhysicsMaterial mat;

    // Movement & Combat Actuators
    uint32_t wheel_count;
    FrogWheel wheels[FROG_MAX_WHEELS];

    uint32_t hitstun_samples; // Active combat stun duration
    float    tagging_factor;  // Momentum clamp on impact (CS 1.6 tagging)
} FrogBody;

// --- [ CORE ENGINE CONTROLS ] ---
void frog_physics_init(void);

// Environment & World control (TIMEPIECE Level properties)
extern FrogPhysicsEnvironment g_frog_physics_env;
void frog_physics_set_environment(vec3_t gravity, float air_density, float water_height);
void frog_physics_set_time_scale(float scale);
void frog_physics_set_hitstop(uint32_t duration_samples);

// Preset loaders for instant prototyping across genres
void frog_physics_load_material_preset(FrogPhysicsMaterial *mat, const char *preset_name);

// Body Configuration & Manipulation
void frog_physics_body_init(FrogBody *body, float mass, float radius);
void frog_physics_body_apply_force(FrogBody *body, vec3_t force);
void frog_physics_body_apply_impulse(FrogBody *body, vec3_t impulse);
void frog_physics_body_tag(FrogBody *body, float velocity_scrub_percent);

// Master 240Hz sub-tick execution driven by AICA sample clock
void frog_physics_update(FrogBody *body, uint32_t delta_samples);
void frog_physics_step_fixed_240hz(FrogBody *body, float dt);

// Universal Movement Models (usable in any game)
void frog_physics_update_wheels(FrogBody *body, float throttle, float steering, float brake, float clutch);
void frog_physics_update_hydrodynamics(FrogBody *body, struct WorldClock *wc);
void frog_physics_update_aerodynamics(FrogBody *body);

// Combat, Platformer & Rhythm Specialized Hooks
void frog_physics_fighting_apply_knockback(FrogBody *attacker, FrogBody *victim, vec3_t force, uint32_t hitstop_samples, struct WorldClock *wc);
void frog_physics_platformer_jump(FrogBody *body, float jump_velocity, bool variable_cut);
float frog_physics_get_wave_height(float x, float z, struct WorldClock *wc);
bool frog_physics_rhythm_check_hit(uint32_t target_sample, uint32_t tolerance, struct WorldClock *wc);
void frog_physics_rhythm_scrub(float stick_deflection, float base_pitch, struct WorldClock *wc);

#ifdef __cplusplus
}
#endif

#endif // FROG_PHYSICS_H
