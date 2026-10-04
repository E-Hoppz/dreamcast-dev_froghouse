#ifndef FROG_VMU_H
#define FROG_VMU_H

#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <stdint.h>

// --- [ VMU LCD ] ---
// The VMU screen is exactly 48x32 monochrome pixels
void frog_vmu_draw_icon(const uint8_t *bitmap);

// --- [ VMU STORAGE ] ---
// Standard "FROGHOUSE" save/load logic
int frog_vmu_save(const char *filename, void *data, int size);
int frog_vmu_load(const char *filename, void *data, int size);

// Helper to find the first available VMU
maple_device_t* frog_vmu_get_device();

#endif
