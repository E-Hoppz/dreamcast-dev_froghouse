//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_loader.h"
#include "frog_util.h"
#include <stdio.h>
#include <stdlib.h>

#define GDROM_SECTOR_SIZE 2048

volatile bool g_g2_bus_locked = false;

bool frog_loader_is_g2_locked(void) {
    return g_g2_bus_locked;
}

bool frog_loader_start(frog_load_task_t *task, const char *path, void *target_buffer) {
    if (!task || !path || !target_buffer) return false;

    task->filename = path;
    task->buffer = target_buffer;
    task->bytes_done = 0;
    task->progress = 0.0f;
    task->status = LOAD_PENDING;

    task->fd = fs_open(path, O_RDONLY);
    if (task->fd == FILEHND_INVALID) {
        task->status = LOAD_ERROR;
        frog_util_log("LOADER ERROR: Failed to open VFS file channel.");
        return false;
    }

    task->size = fs_total(task->fd);
    if (task->size <= 0) {
        task->status = LOAD_ERROR;
        fs_close(task->fd);
        task->fd = FILEHND_INVALID;
        return false;
    }

    return true;
}

void frog_loader_tick(frog_load_task_t *task) {
    if (!task || task->status != LOAD_PENDING) return;

    uint32_t remaining = task->size - task->bytes_done;
    uint32_t chunk_size = (remaining > GDROM_SECTOR_SIZE) ? GDROM_SECTOR_SIZE : remaining;

    uint8_t *dest = (uint8_t *)task->buffer + task->bytes_done;

    g_g2_bus_locked = true;
    int read = fs_read(task->fd, dest, chunk_size);
    g_g2_bus_locked = false;

    if (read <= 0) {
        task->status = LOAD_ERROR;
        frog_util_log("LOADER ERROR: VFS file read failed or unexpected EOF.");
        fs_close(task->fd);
        task->fd = FILEHND_INVALID;
        return;
    }

    task->bytes_done += read;
    task->progress = (float)task->bytes_done / (float)task->size;

    if (task->bytes_done >= task->size) {
        task->status = LOAD_COMPLETE;
        fs_close(task->fd);
        task->fd = FILEHND_INVALID;
        frog_util_log("LOADER: Cooperative time-sliced asset load complete.");
    }
}

void frog_loader_close(frog_load_task_t *task) {
    if (task) {
        if (task->status == LOAD_PENDING && task->fd != FILEHND_INVALID) {
            fs_close(task->fd);
            task->fd = FILEHND_INVALID;
        }
        task->status = LOAD_IDLE;
        task->bytes_done = 0;
        task->progress = 0.0f;
    }
}
