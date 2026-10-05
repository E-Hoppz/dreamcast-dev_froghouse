#ifndef FROG_UTIL_H
#define FROG_UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct WorldClock; // Forward declaration

// --- [ MEMORY HELPERS ] ---
void frog_util_memzero(void *dest, size_t n);

// --- [ LOGIC BREAKERS ] ---
// The Rareware Trick: Skips drawing if the World Clock budget is exceeded.
bool frog_util_should_skip_frame(struct WorldClock *wc);

// --- [ FAST SIGNAL GENERATORS ] ---
// XORShift Math: 3x faster than standard rand()
float frog_util_rand_float();
float frog_rand_range(float min, float max);
int   frog_util_rand_int(int min, int max);

// --- [ MATH & TELEMETRY ] ---
int   frog_is_near(float val1, float val2, float threshold);
void  frog_util_log(const char *msg);

// Fast Fixed-Point to String Shunt for HUDs
void  frog_util_ftoa_shunt(char *out, float val);

#endif
