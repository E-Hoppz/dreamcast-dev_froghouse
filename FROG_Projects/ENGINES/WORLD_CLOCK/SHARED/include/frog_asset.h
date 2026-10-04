#ifndef FROG_ASSET_H
#define FROG_ASSET_H

#include <kos.h>
#include <stdint.h>
#include <dc/pvr.h>
#include "frog_pool.h" 

// --- [ THE TEXTURE COMPONENT ] ---
typedef struct {
    pvr_ptr_t vram_ptr;   
    uint32_t  width;      
    uint32_t  height;     
    uint32_t  format;     
} frog_texture_t;

// --- [ THE PALETTE COMPONENT ] ---
typedef struct {
    uint32_t colors[256]; 
    int      slot;        
} frog_palette_t;

// --- [ THE SHUNT FUNCTIONS ] ---

frog_texture_t* frog_asset_load_pvr(const char *filename);
frog_texture_t* frog_asset_load_to_pool(const char *filename, frog_pool_t *pool);
void frog_asset_load_palette(frog_palette_t *pal);
void frog_asset_unload(frog_texture_t *tex);

#endif
