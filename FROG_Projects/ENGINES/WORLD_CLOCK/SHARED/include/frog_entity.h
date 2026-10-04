#ifndef FROG_ENTITY_H
#define FROG_ENTITY_H

#include "frog_physics.h"
#include "frog_animate.h"
#include "frog_scene.h"
#include "frog_state.h"
#include "world_clock.h"
#include <stdbool.h>

#define MAX_TARGET_NAME 32
#define MAX_ENTITIES 128
#define MAX_SCHEDULED_EVENTS 64

typedef enum {
    ENT_PLAYER,
    ENT_NPC,
    ENT_TRIGGER_ONCE,
    ENT_TRIGGER_MULTIPLE,
    ENT_SCRIPTED_SEQUENCE,
    ENT_MULTI_MANAGER
} FrogEntityType;

typedef struct FrogEntity {
    char targetname[MAX_TARGET_NAME];
    char target[MAX_TARGET_NAME];
    FrogEntityType type;
    bool active;

    // Component Rails
    FrogBody body;               // 64-byte Cache-aligned physics component
    frog_state_t state;          // SPU-Clock locked state controller
    frog_node_t scene_node;      // Graph hierarchy draw node
    FrogAnimationClip *anim_clip; // Visual movement state reference
    FrogAnimator animator;       // Skeletal sequence timer

    // --- [ NEW MODULAR ENTITY PLUGINS ] ---
    // Game-specific behavior hooks. Allows any entity to bind and execute 
    // custom, combined physics and thinking functions on the fly.
    void (*physics_update)(struct FrogEntity *self, WorldClock *wc);
    void (*think)(struct FrogEntity *self, WorldClock *wc);

    // Configuration data fields 
    union {
        struct {
            float trigger_radius;
            bool triggered;
        } trigger;
        struct {
            char anim_name[32];
            uint32_t duration_samples; // Script track lifespan in SPU samples
            char next_target[MAX_TARGET_NAME];
            bool triggered;
        } script;
        struct {
            uint32_t trigger_delay_ms;
        } multi_manager;
    } data;

    // Trigger action callback pointer
    void (*on_trigger)(struct FrogEntity *self, struct FrogEntity *activator, WorldClock *wc);
} FrogEntity;

typedef struct {
    char target[MAX_TARGET_NAME];
    uint32_t target_sample; // Resolved absolute future monotonic SPU sample point
    bool active;
} FrogScheduledEvent;

extern FrogEntity g_entities[MAX_ENTITIES];
extern uint32_t g_entity_count;

extern FrogScheduledEvent g_scheduled_events[MAX_SCHEDULED_EVENTS];

void frog_entity_system_init(void);
FrogEntity* frog_entity_spawn(FrogEntityType type, const char *targetname, const char *target);
void frog_entity_fire_targets(const char *targetname, FrogEntity *activator, WorldClock *wc);
void frog_entity_schedule_event(const char *targetname, float delay_seconds, WorldClock *wc);
void frog_entity_update_system(WorldClock *wc);

#endif
