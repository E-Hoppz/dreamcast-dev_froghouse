//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_loader.h"
#include "frog_util.h"
#include <stdio.h>
#include <stdlib.h>

#define GDROM_SECTOR_SIZE 2048 // Standard physical CD-ROM sector size in bytes

// Global bus synchronization flag to communicate G2 state to the World Clock
volatile bool g_g2_bus_locked = false;

bool frog_loader_start(frog_load_task_t *task, const char *path, void *target_buffer) {
    if (!task || !path || !target_buffer) return false;

    task->filename = path;
    task->buffer = target_buffer;
    task->bytes_done = 0;
    task->progress = 0.0f;
    task->status = LOAD_PENDING;

    // Open file descriptor in non-blocking standard read mode
    task->fd = fs_open(path, O_RDONLY);
    if (task->fd == FILEHND_INVALID) {
        task->status = LOAD_ERROR;
        frog_util_log("LOADER ERROR: Failed to open VFS file channel.");
        return false;
    }

    task->size = fs_total(task->fd);
    return true;
}

void frog_loader_tick(frog_load_task_t *task) {
    if (!task || task->status != LOAD_PENDING) return;

    // [COOPERATIVE TIME-SLICING]
    // We read exactly 1 physical sector (2048 bytes) per frame.
    // This guarantees the SH-4 never halts or spikes wc->delta_time!
    uint32_t remaining = task->size - task->bytes_done;
    uint32_t chunk_size = (remaining > GDROM_SECTOR_SIZE) ? GDROM_SECTOR_SIZE : remaining;

    uint8_t *dest = (uint8_t *)task->buffer + task->bytes_done;

    // Raise the G2 bus lock to protect SPU register read operations
    g_g2_bus_locked = true;
    int read = fs_read(task->fd, dest, chunk_size);
    g_g2_bus_locked = false;

    if (read < 0) {
        task->status = LOAD_ERROR;
        frog_util_log("LOADER ERROR: VFS file read failed.");
        fs_close(task->fd);
        return;
    }

    task->bytes_done += read;
    task->progress = (float)task->bytes_done / (float)task->size;

    if (task->bytes_done >= task->size) {
        task->status = LOAD_COMPLETE;
        fs_close(task->fd);
        frog_util_log("LOADER: Cooperative time-sliced asset load complete.");
    }
}

void frog_loader_close(frog_load_task_t *task) {
    if (task) {
        if (task->status == LOAD_PENDING && task->fd != FILEHND_INVALID) {
            fs_close(task->fd);
        }
        task->status = LOAD_IDLE;
        task->bytes_done = 0;
        task->progress = 0.0f;
    }
}
