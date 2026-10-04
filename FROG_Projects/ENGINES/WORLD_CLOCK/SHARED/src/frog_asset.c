#include "frog_asset.h"
#include "frog_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>  // Guarantees cross-platform definition of uintptr_t

/**
 * Loads a native PVR texture file directly into the primary VRAM pool.
 */
frog_texture_t* frog_asset_load_pvr(const char *filename) {
    file_t f;
    uint32_t header[4]; 
    
    f = fs_open(filename, O_RDONLY);
    if (f == FILEHND_INVALID) {
        frog_util_log("ASSET ERROR: File not found.");
        return NULL;
    }

    fs_seek(f, 8, SEEK_SET);
    fs_read(f, header, 8); 

    frog_texture_t *tex = (frog_texture_t*)malloc(sizeof(frog_texture_t));
    if (!tex) {
        fs_close(f);
        return NULL;
    }
    
    tex->format = header[0];
    tex->width  = header[1];
    tex->height = header[2];

    uint32_t size = tex->width * tex->height * 2;

    tex->vram_ptr = pvr_mem_malloc(size);
    if (!tex->vram_ptr) {
        frog_util_log("ASSET ERROR: VRAM Vault is full.");
        fs_close(f);
        free(tex);
        return NULL;
    }

    fs_seek(f, 16, SEEK_SET);
    
    void *temp_buf = malloc(size);
    if (!temp_buf) {
        pvr_mem_free(tex->vram_ptr);
        fs_close(f);
        free(tex);
        return NULL;
    }
    
    fs_read(f, temp_buf, size);
    pvr_txr_load(temp_buf, tex->vram_ptr, size);
    
    free(temp_buf);
    fs_close(f);

    frog_util_log("ASSET: PVR shunted to VRAM successfully.");
    return tex;
}

/**
 * Loads a texture file directly into a custom memory arena (System, AICA, or VRAM)
 * using the aligned, platform-safe allocation pool.
 */
frog_texture_t* frog_asset_load_to_pool(const char *filename, frog_pool_t *pool) {
    if (!pool) return NULL;

    file_t f = fs_open(filename, O_RDONLY);
    if (f == FILEHND_INVALID) {
        frog_util_log("ASSET ERROR: Pool asset not found.");
        return NULL;
    }

    uint32_t header[4];
    fs_seek(f, 8, SEEK_SET);
    fs_read(f, header, 8);

    frog_texture_t *tex = (frog_texture_t*)malloc(sizeof(frog_texture_t));
    if (!tex) {
        fs_close(f);
        return NULL;
    }
    
    tex->format = header[0];
    tex->width  = header[1];
    tex->height = header[2];

    uint32_t size = tex->width * tex->height * 2;
    
    // --- [ COUPLING FIX: Capture pool offset using uintptr_t ] ---
    uintptr_t allocated_addr = frog_pool_alloc(pool, size);
    if (!allocated_addr) {
        frog_util_log("ASSET ERROR: Pool allocation failed.");
        fs_close(f);
        free(tex);
        return NULL;
    }

    // Explicitly cast the uintptr_t back to the PVR hardware pointer type
    tex->vram_ptr = (pvr_ptr_t)allocated_addr;
    fs_seek(f, 16, SEEK_SET);
    
    void *temp_buf = malloc(size);
    if (!temp_buf) {
        fs_close(f);
        free(tex);
        return NULL;
    }
    
    fs_read(f, temp_buf, size);

    if (pool->type == POOL_VRAM) {
        pvr_txr_load(temp_buf, tex->vram_ptr, size);
    } else {
        memcpy((void*)tex->vram_ptr, temp_buf, size);
    }
    
    free(temp_buf);
    fs_close(f);

    frog_util_log("ASSET: Texture mapped to Pool memory.");
    return tex;
}

/**
 * Assigns a 256-color ARGB palette configuration to the physical PVR hardware.
 */
void frog_asset_load_palette(frog_palette_t *pal) {
    if (!pal) return;
    pvr_set_pal_format(PVR_PAL_ARGB8888);
    for (int i = 0; i < 256; i++) {
        pvr_set_pal_entry(pal->slot * 256 + i, pal->colors[i]);
    }
    frog_util_log("ASSET: Palette mapped to hardware registers.");
}

/**
 * Safely unbinds and deallocates texture memory from the systems.
 */
void frog_asset_unload(frog_texture_t *tex) {
    if (tex) {
        if (tex->vram_ptr) pvr_mem_free(tex->vram_ptr);
        free(tex);
    }
}
