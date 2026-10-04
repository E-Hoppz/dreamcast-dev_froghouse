#ifndef FROG_STATE_H
#define FROG_STATE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * [THE LOGIC GATE]
 * This structure acts as a hardware relay. If 'locked' is true, 
 * the state cannot be changed until the 'duration' (in AICA samples) elapses.
 */
typedef struct {
    int current;             // The active logic path
    int previous;            // For "Rewind" or "State-Exit" logic
    bool locked;             // If true, rejection of state changes is active
    uint32_t start_sample;   // THE IGNITION: When this state was triggered
    uint32_t duration;       // THE LIFESPAN: How many samples until it unlocks
} frog_state_t;

// [THE IGNITION SWITCH]
// Attempts to flip the relay to a new state.
// current_sample: Must be passed from wc.audio.sample_count to ignite the timer.
bool frog_state_try_set(frog_state_t *fsm, int new_state, bool lock, uint32_t sample_len, uint32_t current_sample);

// [THE SIGNAL CHECK]
// Checks if the World Clock has bypassed the duration limit to unlock the relay.
void frog_state_tick(frog_state_t *fsm, uint32_t current_world_sample);

// [THE PROGRESS SHUNT]
// Returns 0.0 to 1.0 based on how much of the duration has passed.
// Perfect for "Mano de Oro" visual interpolation and smooth racing transitions.
float frog_state_get_progress(frog_state_t *fsm, uint32_t current_world_sample);

#endif
