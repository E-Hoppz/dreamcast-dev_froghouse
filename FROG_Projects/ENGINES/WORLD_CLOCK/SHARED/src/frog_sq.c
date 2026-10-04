#include <dc/sq.h>
#include "frog_util.h"

/**
 * [THE HIGH-SPEED SHUNT]
 * Burst-fires data from System RAM to VRAM or AICA.
 * This is 2x faster than standard memcpy.
 */
void frog_sq_shunt(void *dest, const void *src, size_t n) {
    // [REPAIRMAN'S TRUTH] We must be 32-byte aligned for the SQ to engage.
    // This is why we built frog_pool with 32-byte alignment!
    sq_cpy(dest, src, n); 
}

void frog_sq_clear(void *dest, size_t n) {
    // Rapid-zeroing of VRAM for new scene transitions.
    sq_clr(dest, n);
}
