#include "frog_entity.h"
#include "frog_physics.h"
#include "frog_animate.h"
#include "frog_matrix.h"
#include "frog_util.h"
#include "world_clock.h"
#include <string.h>
#include <stdlib.h>

FrogEntity g_entities[MAX_ENTITIES];
uint32_t g_entity_count = 0;

FrogScheduledEvent g_scheduled_events[MAX_SCHEDULED_EVENTS];

/**
 * Clean system allocation tables.
 */
void frog_entity_system_init(void) {
    memset(g_entities, 0, sizeof(g_entities));
    g_entity_count = 0;
    
    memset(g_scheduled_events, 0, sizeof(g_scheduled_events));
    frog_util_log("ENTITY: Event Manager and Scripting Scheduler Active.");
}

/**
 * Reserves space in the global entity heap.
 */
FrogEntity* frog_entity_spawn(FrogEntityType type, const char *targetname, const char *target) {
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!g_entities[i].active) {
            FrogEntity *ent = &g_entities[i];
            memset(ent, 0, sizeof(FrogEntity));

            ent->active = true;
            ent->type = type;
            
            if (targetname) {
                strncpy(ent->targetname, targetname, MAX_TARGET_NAME - 1);
            }
            if (target) {
                strncpy(ent->target, target, MAX_TARGET_NAME - 1);
            }

            // Initialize behavior callbacks to NULL by default
            ent->physics_update = NULL;
            ent->think = NULL;

            frog_node_init(&ent->scene_node);
            g_entity_count++;
            return ent;
        }
    }
    frog_util_log("ENTITY WARNING: Spawn request failed. Max capacity reached.");
    return NULL;
}

/**
 * Searches the runtime registry and routes trigger signals to matching target targets.
 */
void frog_entity_fire_targets(const char *targetname, FrogEntity *activator, WorldClock *wc) {
    if (!targetname || strlen(targetname) == 0) return;

    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!g_entities[i].active) continue;

        // Reset cooldown status of trigger_multiple nodes when their targeting loop completes
        if (g_entities[i].type == ENT_TRIGGER_MULTIPLE && strcmp(g_entities[i].targetname, targetname) == 0) {
            g_entities[i].data.trigger.triggered = false;
        }

        if (strcmp(g_entities[i].targetname, targetname) == 0) {
            
            // --- [ REACTIONARY STATE MACHINE OVERRIDE ] ---
            if (g_entities[i].type == ENT_SCRIPTED_SEQUENCE) {
                // Find target NPCs mapped to this scripted sequence's parameter fields
                for (int j = 0; j < MAX_ENTITIES; j++) {
                    if (g_entities[j].active && g_entities[j].type == ENT_NPC &&
                        strcmp(g_entities[j].targetname, g_entities[i].target) == 0) {
                        
                        // Force NPC state change to Script Mode (State 2)
                        bool success = frog_state_try_set(
                            &g_entities[j].state, 
                            2, // Locked script mode path
                            true, // Locked status active
                            g_entities[i].data.script.duration_samples, 
                            wc->audio.monotonic_samples
                        );

                        if (success) {
                            frog_util_log("SCRIPT: Target NPC locked into scripted animation state.");
                            
                            // Synchronize next sequential target using SPU timing limits
                            if (strlen(g_entities[i].data.script.next_target) > 0) {
                                float delay = (float)g_entities[i].data.script.duration_samples / 44100.0f;
                                frog_entity_schedule_event(g_entities[i].data.script.next_target, delay, wc);
                            }
                        }
                    }
                }
            } 

            // Execute custom trigger callback if available
            if (g_entities[i].on_trigger) {
                g_entities[i].on_trigger(&g_entities[i], activator, wc);
            }
        }
    }
}

/**
 * [THE SPU-SLAVED SCHEDULER]
 * Adds delayed signals to the queue to trigger at an exact future monotonic sample count.
 */
void frog_entity_schedule_event(const char *targetname, float delay_seconds, WorldClock *wc) {
    if (!targetname || strlen(targetname) == 0) return;

    for (int i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        if (!g_scheduled_events[i].active) {
            strncpy(g_scheduled_events[i].target, targetname, MAX_TARGET_NAME - 1);
            
            // target_sample = current continuous playhead + delay calculated to 44.1kHz standard
            uint32_t samples_delay = (uint32_t)(delay_seconds * 44100.0f);
            g_scheduled_events[i].target_sample = wc->audio.monotonic_samples + samples_delay;
            g_scheduled_events[i].active = true;
            return;
        }
    }
    frog_util_log("EVENT SCHEDULER: Delayed event buffer overflow. Queue rejected.");
}

/**
 * Ticks scheduled events, handles proximity trigger bounds, and syncs physics transforms.
 */
void frog_entity_update_system(WorldClock *wc) {
    uint32_t current_sample = wc->audio.monotonic_samples;

    // 1. Tick scheduled event sequences
    for (int i = 0; i < MAX_SCHEDULED_EVENTS; i++) {
        if (g_scheduled_events[i].active) {
            // Uses subtraction to prevent monotonic SPU count wrap failures
            int32_t delta = (int32_t)(current_sample - g_scheduled_events[i].target_sample);
            if (delta >= 0) {
                g_scheduled_events[i].active = false;
                frog_entity_fire_targets(g_scheduled_events[i].target, NULL, wc);
            }
        }
    }

    // 2. Update physics, graph mappings, and trigger coordinates
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!g_entities[i].active) continue;

        // --- [ THE DYNAMIC PLUGINS STAGE ] ---
        // Run game-specific movement/thinking behaviors prior to compiling transformation matrices
        if (g_entities[i].physics_update != NULL) {
            g_entities[i].physics_update(&g_entities[i], wc);
        }
        if (g_entities[i].think != NULL) {
            g_entities[i].think(&g_entities[i], wc);
        }

        // Tick internal state sequence limits
        frog_state_tick(&g_entities[i].state, current_sample);

        // Standard coordinate update to parent draw node using direct array memory copy
        mat4_t trans = mat4_translate(
            g_entities[i].body.position.x,
            g_entities[i].body.position.y,
            g_entities[i].body.position.z
        );
        memcpy(&g_entities[i].scene_node.local_matrix, &trans.m, sizeof(matrix_t));

        // Check trigger volumes against player position (assumed to be index 0)
        if (i > 0 && g_entities[0].active && 
            (g_entities[i].type == ENT_TRIGGER_ONCE || g_entities[i].type == ENT_TRIGGER_MULTIPLE)) {
            
            float distance = vec3_length(vec3_sub(g_entities[0].body.position, g_entities[i].body.position));
            if (distance < g_entities[i].data.trigger.trigger_radius) {
                if (g_entities[i].type == ENT_TRIGGER_ONCE && !g_entities[i].data.trigger.triggered) {
                    g_entities[i].data.trigger.triggered = true;
                    frog_entity_fire_targets(g_entities[i].target, &g_entities[0], wc);
                } 
                else if (g_entities[i].type == ENT_TRIGGER_MULTIPLE && !g_entities[i].data.trigger.triggered) {
                    g_entities[i].data.trigger.triggered = true;
                    frog_entity_fire_targets(g_entities[i].target, &g_entities[0], wc);
                    
                    // Fire a re-enabler delayed loop after a 2-second cooldown period
                    frog_entity_schedule_event(g_entities[i].targetname, 2.0f, wc);
                }
            } else {
                // Clear active trigger status once actor leaves proximity limits
                if (g_entities[i].type == ENT_TRIGGER_MULTIPLE) {
                    g_entities[i].data.trigger.triggered = false;
                }
            }
        }
    }
}
