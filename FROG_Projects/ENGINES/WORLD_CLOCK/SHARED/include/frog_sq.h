#ifndef FROG_SQ_H
#define FROG_SQ_H

#include <stddef.h>
#include <stdint.h>

/**
 * [THE OVERDRIVE SHUNT]
 * Burst-fires data from System RAM to VRAM or AICA Vault.
 * Requirement: Both src and dest MUST be 32-byte aligned.
 */
void frog_sq_shunt(void *dest, const void *src, size_t n);

/**
 * [THE RAPID CLEAR]
 * Zeroes out a block of memory (VRAM or RAM) at hardware speed.
 */
void frog_sq_clear(void *dest, size_t n);

/**
 * [THE VERTEX BURST]
 * Shunts a single 32-byte vertex directly to the PVR buffer.
 * This is the secret to 10,000+ particles.
 */
void frog_sq_send_vertex(void *vert);

#endif
