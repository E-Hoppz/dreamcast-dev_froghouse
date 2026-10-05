#ifndef FROG_FONT_H
#define FROG_FONT_H

#include <stdint.h>
#include "frog_asset.h"
#include "frog_draw.h"

struct WorldClock;

/**
 * [THE SHUNT]
 * Loads any standard 16x16 ASCII font texture into the VRAM Vault.
 */
frog_texture_t* frog_font_load(const char *texture_path);

/**
 * [THE OUTPUT]
 * High-speed hardware text blitting using direct PVR UV cell-slicing.
 */
void frog_font_print(float x, float y, float scale, uint32_t color, 
                     frog_texture_t *font_tex, struct WorldClock *wc, 
                     const char *text);

/**
 * [MAINTENANCE]
 * De-solders the font from VRAM.
 */
void frog_font_unload(frog_texture_t *font_tex);

#endif
