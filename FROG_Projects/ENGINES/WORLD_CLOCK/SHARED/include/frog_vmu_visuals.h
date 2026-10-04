#ifndef FROG_VMU_VISUALS_H
#define FROG_VMU_VISUALS_H

#include <stdint.h>
#include "world_clock.h"

// The VMU screen is 48x32 monochrome (1 bit per pixel)
// 48 * 32 / 8 bits = 192 bytes per frame
typedef struct {
    uint8_t bitmap[192];
} FrogVMUFrame;

/**
 * [THE HANDHELD SHUNT]
 * Automatically checks the World Clock. If the bass peak is high enough, 
 * it sends an image to the controller screen.
 */
void frog_vmu_vibe_sync(WorldClock *wc, const uint8_t *icon_on, const uint8_t *icon_off);

/**
 * [THE SENSITIVITY CALIBRATOR]
 * Allows real-time tuning of the sub-bass visualization triggers.
 */
void frog_vmu_set_sensitivity(float val);

#endif
