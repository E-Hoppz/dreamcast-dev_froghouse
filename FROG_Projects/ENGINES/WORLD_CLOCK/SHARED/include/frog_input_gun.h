#ifndef FROG_INPUT_GUN_H
#define FROG_INPUT_GUN_H

#include <stdint.h>
#include <stdbool.h>
#include "world_clock.h"
#include "frog_math_3d.h"

typedef struct {
    bool     trigger_pulled;    // Digital trigger signal
    bool     offscreen;         // Offscreen flash flag
    int      screen_x;          // Hardware raster X-coordinate
    int      screen_y;          // Hardware raster Y-coordinate
    uint32_t trigger_sample;    // Exact SPU sample point when trigger fired
} FrogLightgun;

/**
 * [THE LIGHTGUN SHUNT]
 * Polls the physical Maple lightgun hardware. Maps coordinate offsets
 * and stamps the trigger execution with your exact SPU sample clock.
 */
void frog_input_gun_update(FrogLightgun *gun, WorldClock *wc);

/**
 * [THE BULLET RAYCAST]
 * Translates 2D screen raster coordinates into a 3D ray vector slaved to SPU timing.
 */
void frog_input_gun_get_ray(FrogLightgun *gun, vec3_t *out_origin, vec3_t *out_dir);

#endif
