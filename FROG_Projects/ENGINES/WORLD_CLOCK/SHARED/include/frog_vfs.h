#ifndef FROG_VFS_H
#define FROG_VFS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char     asset_name[32]; // Unique file ID
    uint32_t start_offset;   // Byte offset inside assets.pack
    uint32_t size;           // Total asset size in bytes
} FrogPackEntry;

typedef struct {
    uint32_t      asset_count;
    FrogPackEntry *catalog;
} FrogPackVFS;

/**
 * [THE VFS IGNITION]
 * Opens 'assets.pack' and caches the lightweight TOC (Table of Contents) 
 * into system RAM to enable zero-wait-state file seeks.
 */
bool frog_vfs_init(const char *pack_path);

/**
 * [ZERO-METADATA LOOKUP]
 * Finds an asset's entry by its name using a fast binary search.
 */
bool frog_vfs_locate(const char *name, uint32_t *out_offset, uint32_t *out_size);

/**
 * [CLOSE PIPELINE]
 */
void frog_vfs_shutdown(void);

#endif
