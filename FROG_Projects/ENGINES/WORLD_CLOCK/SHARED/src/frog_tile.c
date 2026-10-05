//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_tile.h"
#include "frog_sq.h"
#include "frog_draw.h"
#include "frog_util.h" 
#include "frog_config.h"
#include <dc/pvr.h>
#include <dc/sq.h>
#include <math.h>

void frog_tile_init(FrogTileMap *map, uint32_t w, uint32_t h, uint32_t size, frog_texture_t *tex, uint16_t *map_data) {
    if (!map) return;
    map->width = w;
    map->height = h;
    map->tile_size = size;
    map->tileset = tex;
    map->data = map_data;
    frog_util_log("TILEMAP: Structure calibrated and bound successfully.");
}

// Optimized tile rendering eliminating division and modulo operations inside the frame loop
void frog_tile_render(FrogTileMap *map, float scroll_x, float scroll_y, bool filter, WorldClock *wc) {
    if (!map || !map->tileset) return;

    FrogVibe vibe = filter ? VIBE_SONIC : VIBE_SATURN;
    int list_type = (vibe == VIBE_SATURN) ? PVR_LIST_PT_POLY : PVR_LIST_OP_POLY;
    
    pvr_list_begin(list_type);
    
    int filter_mode = (frog_engine_config.active_contract == CONTRACT_GLIDE) ? PVR_FILTER_BILINEAR : (filter ? PVR_FILTER_BILINEAR : PVR_FILTER_NONE);

    pvr_poly_cxt_t cxt;
    pvr_poly_cxt_txr(&cxt, list_type, map->tileset->format, map->tileset->width, 
                     map->tileset->height, map->tileset->vram_ptr, 
                     filter_mode);
    
    pvr_poly_hdr_t map_hdr;
    pvr_poly_compile(&map_hdr, &cxt);
    pvr_prim(&map_hdr, sizeof(pvr_poly_hdr_t));

    int start_tile_x = (int)(scroll_x / map->tile_size);
    int start_tile_y = (int)(scroll_y / map->tile_size);
    float offset_x = -(fmodf(scroll_x, map->tile_size));
    float offset_y = -(fmodf(scroll_y, map->tile_size));

    // Calculate dimensions once to bypass division inside nested loops
    register int tiles_per_row = map->tileset->width / map->tile_size;
    register float u_step = (float)map->tile_size / (float)map->tileset->width;
    register float v_step = (float)map->tile_size / (float)map->tileset->height;

    pvr_dr_state_t dr_state __attribute__((unused)) = 0;
    pvr_vertex_t *vert;

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 21; x++) {
            int map_x = start_tile_x + x;
            int map_y = start_tile_y + y;

            if (map_x >= map->width || map_y >= map->height) continue;

            uint16_t tile_id = map->data[map_y * map->width + map_x];
            
            // Division-free columns and rows calculations
            uint32_t tile_row = tile_id / tiles_per_row;
            uint32_t tile_col = tile_id - (tile_row * tiles_per_row);

            float u = (float)tile_col * u_step;
            float v = (float)tile_row * v_step;

            float dx = offset_x + (x * map->tile_size);
            float dy = offset_y + (y * map->tile_size);

            // Vertex 1
            vert = pvr_dr_target(dr_state);
            vert->flags = PVR_CMD_VERTEX;
            vert->x = dx; vert->y = dy + map->tile_size; vert->z = 1.0f; vert->u = u; vert->v = v + v_step;
            vert->argb = 0xFFFFFFFF; vert->oargb = 0;
            pvr_dr_commit(vert);

            // Vertex 2
            vert = pvr_dr_target(dr_state);
            vert->flags = PVR_CMD_VERTEX;
            vert->x = dx; vert->y = dy; vert->z = 1.0f; vert->u = u; vert->v = v;
            vert->argb = 0xFFFFFFFF; vert->oargb = 0;
            pvr_dr_commit(vert);

            // Vertex 3
            vert = pvr_dr_target(dr_state);
            vert->flags = PVR_CMD_VERTEX;
            vert->x = dx + map->tile_size; vert->y = dy + map->tile_size; vert->z = 1.0f; vert->u = u + u_step; vert->v = v + v_step;
            vert->argb = 0xFFFFFFFF; vert->oargb = 0;
            pvr_dr_commit(vert);

            // Vertex 4 (EOL)
            vert = pvr_dr_target(dr_state);
            vert->flags = PVR_CMD_VERTEX_EOL;
            vert->x = dx + map->tile_size; vert->y = dy; vert->z = 1.0f; vert->u = u + u_step; vert->v = v;
            vert->argb = 0xFFFFFFFF; vert->oargb = 0;
            pvr_dr_commit(vert);
        }
    }
}
