#include "frog_util.h"
#include "world_clock.h"
#include <stdio.h>
#include <stdlib.h>
#include <dc/sq.h>

// [REPAIRMAN'S NITRO] Fast Random Signal Generator (XORShift)
// Standard rand() is a heavy-draw component. This is pure bit-shifting speed.
static uint32_t rng_state = 0xDEADC0DE;

void frog_util_memzero(void *dest, size_t n) {
    // Hardware Clear: 32-byte burst fire
    sq_clr(dest, n); 
}

/**
 * [THE VOLTAGE REGULATOR: VBLANK SKIP]
 * Checks if we are over-budget for the current frame.
 * 735 samples = 1/60th of a second.
 * If we exceed this, we skip rendering to keep the World Clock from drifting.
 */
bool frog_util_should_skip_frame(WorldClock *wc) {
    uint32_t frame_budget = 735; // 44100 / 60
    uint32_t elapsed = wc->audio.sample_count - wc->audio.current_offset;
    
    return (elapsed > frame_budget);
}

// [THE FAST SIGNAL GENERATOR]
float frog_util_rand_float() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (float)(rng_state & 0xFFFFFF) / 16777216.0f;
}

float frog_rand_range(float min, float max) {
    return min + (frog_util_rand_float() * (max - min));
}

int frog_util_rand_int(int min, int max) {
    return min + (int)(frog_util_rand_float() * (max - min + 1));
}

int frog_is_near(float val1, float val2, float threshold) {
    return fabsf(val1 - val2) < threshold;
}

void frog_util_log(const char *msg) {
    printf("[FROGHOUSE]: %s\n", msg);
}

void frog_util_ftoa_shunt(char *out, float val) {
    // Fast BCD Shunt: Bypasses sprintf's heavy CPU draw
    int ival = (int)(val * 100.0f);
    if (ival < 0) ival = 0; 
    out[0] = (ival / 100) + '0';
    out[1] = '.';
    out[2] = ((ival / 10) % 10) + '0';
    out[3] = (ival % 10) + '0';
    out[4] = '\0';
}
