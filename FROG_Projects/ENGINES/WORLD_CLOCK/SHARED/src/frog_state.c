#include "frog_state.h"
#include "world_clock.h"

// [REPAIRMAN'S FIX] We must pass the current_sample to "ignite" the timer
bool frog_state_try_set(frog_state_t *fsm, int new_state, bool lock, uint32_t sample_len, uint32_t current_sample) {
    // If the relay is locked (e.g., in Hit-Stun or mid-Gear Shift), reject the change
    if(fsm->locked) return false;
    
    fsm->previous = fsm->current;
    fsm->current = new_state;
    fsm->locked = lock;
    fsm->duration = sample_len;
    
    // [THE IGNITION] Capture the exact "Mechanical Truth" the state started
    fsm->start_sample = current_sample; 
    
    return true;
}

// [THE PROGRESS SHUNT] Returns 0.0 to 1.0 based on how far through the state we are
// Perfect for: 
// - Smoothly shifting a Racing camera
// - Interpolating a Fighter's limb from Startup to Active
// - Fading a Rhythm note window
float frog_state_get_progress(frog_state_t *fsm, uint32_t current_sample) {
    if (fsm->duration == 0) return 1.0f;
    
    // Calculate elapsed samples (handles unsigned wraparound automatically)
    uint32_t elapsed = current_sample - fsm->start_sample;
    
    float progress = (float)elapsed / (float)fsm->duration;
    return (progress > 1.0f) ? 1.0f : progress;
}

void frog_state_tick(frog_state_t *fsm, uint32_t current_sample) {
    if(!fsm->locked) return;

    // Logic Check: Has the World Clock ticked past the intended duration?
    // Using unsigned subtraction here is like a hardware "wrap" check—it's robust.
    if(current_sample - fsm->start_sample >= fsm->duration) {
        fsm->locked = false;
    }
}
