#include "frog_vfs.h"
#include "frog_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kos/fs.h>

static file_t pack_fd = FILEHND_INVALID;
static FrogPackVFS vfs_instance = {0, NULL};

bool frog_vfs_init(const char *pack_path) {
    if (pack_fd != FILEHND_INVALID) {
        fs_close(pack_fd);
    }

    pack_fd = fs_open(pack_path, O_RDONLY);
    if (pack_fd == FILEHND_INVALID) {
        frog_util_log("VFS ERROR: Failed to open master assets.pack");
        return false;
    }

    // Read the asset count at the beginning of the file (first 4 bytes)
    if (fs_read(pack_fd, &vfs_instance.asset_count, sizeof(uint32_t)) != sizeof(uint32_t)) {
        frog_util_log("VFS ERROR: Failed to read asset count from header.");
        fs_close(pack_fd);
        pack_fd = FILEHND_INVALID;
        return false;
    }

    // Allocate memory from standard heap for the TOC catalog
    size_t catalog_size = vfs_instance.asset_count * sizeof(FrogPackEntry);
    vfs_instance.catalog = (FrogPackEntry *)malloc(catalog_size);
    if (!vfs_instance.catalog) {
        frog_util_log("VFS ERROR: Out of memory allocating TOC catalog.");
        fs_close(pack_fd);
        pack_fd = FILEHND_INVALID;
        return false;
    }

    // Read the compiled catalog block following the header
    if (fs_read(pack_fd, vfs_instance.catalog, catalog_size) != catalog_size) {
        frog_util_log("VFS ERROR: Failed to load catalog table.");
        free(vfs_instance.catalog);
        vfs_instance.catalog = NULL;
        fs_close(pack_fd);
        pack_fd = FILEHND_INVALID;
        return false;
    }

    frog_util_log("VFS: Assets package mounted successfully.");
    return true;
}

bool frog_vfs_locate(const char *name, uint32_t *out_offset, uint32_t *out_size) {
    if (pack_fd == FILEHND_INVALID || !vfs_instance.catalog || vfs_instance.asset_count == 0) {
        return false;
    }

    // Binary search algorithm for locating packed asset offsets
    int low = 0;
    int high = (int)vfs_instance.asset_count - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(name, vfs_instance.catalog[mid].asset_name);

        if (cmp == 0) {
            *out_offset = vfs_instance.catalog[mid].start_offset;
            *out_size = vfs_instance.catalog[mid].size;
            return true;
        } else if (cmp < 0) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    return false; // Asset not found
}

void frog_vfs_shutdown(void) {
    if (vfs_instance.catalog) {
        free(vfs_instance.catalog);
        vfs_instance.catalog = NULL;
    }
    vfs_instance.asset_count = 0;

    if (pack_fd != FILEHND_INVALID) {
        fs_close(pack_fd);
        pack_fd = FILEHND_INVALID;
    }
}
