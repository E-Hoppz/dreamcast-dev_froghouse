#ifndef FROG_SCRATCHPAD_H
#define FROG_SCRATCHPAD_H

#include <stdint.h>
#include <stddef.h>

/**
 * [THE HARDWARE EXPLOIT: SCRATCHPAD RAM]
 * Reconfigures the SH-4 Cache Control Register (CCR) to split the 
 * 8KB Data Cache in half, turning 4KB of it into zero-latency Scratchpad RAM.
 * Physically mapped to 0x7c000000 to 0x7c000fff on the CPU die.
 */
void  frog_scratchpad_init(void);

// Zero-wait-state bump allocator inside the 4KB scratchpad
void* frog_scratchpad_alloc(size_t size);
void  frog_scratchpad_reset(void);

#endif
