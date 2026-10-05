#ifndef FROG_TILE_H
#define FROG_TILE_H

#include <stdint.h>
#include <stdbool.h>
#include "frog_asset.h"
#include "world_clock.h"

/**
 * [THE PARALLAX COMPONENT]
 */
typedef struct {
    uint32_t width, height;  
    uint32_t tile_size;      
    uint16_t *data;          
    frog_texture_t *tileset; 
} FrogTileMap;

/**
 * [THE ASSEMBLER]
 */
void frog_tile_init(FrogTileMap *map, uint32_t w, uint32_t h, uint32_t size, frog_texture_t *tex, uint16_t *map_data);

/**
 * [THE SQ FIREHOSE RENDER]
 * Blasts visible tiles to PVR with correct direct store queue mappings.
 */
void frog_tile_render(FrogTileMap *map, float scroll_x, float scroll_y, bool filter, WorldClock *wc);

#endif
