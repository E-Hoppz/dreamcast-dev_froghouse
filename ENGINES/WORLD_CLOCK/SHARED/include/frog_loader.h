#ifndef FROG_LOADER_H
#define FROG_LOADER_H

#include <kos.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LOAD_IDLE,      
    LOAD_PENDING,   
    LOAD_COMPLETE,  
    LOAD_ERROR      
} frog_load_status_t;

typedef struct {
    const char *filename;   
    void       *buffer;     // Target RAM destination
    uint32_t    size;       // Total file size in bytes
    uint32_t    bytes_done; // Current loaded byte offset
    float       progress;   // 0.0 to 1.0 (For loading-bar visualization)
    
    frog_load_status_t status;
    file_t fd;              // Active KOS file descriptor
} frog_load_task_t;

// Global bus synchronization flag communicating GD-ROM G2 bus lock state
extern volatile bool g_g2_bus_locked;

/**
 * [THE COOPERATIVE IGNITION]
 * Initializes a non-blocking, cooperative file load task.
 * Opens the file descriptor safely.
 */
bool frog_loader_start(frog_load_task_t *task, const char *path, void *target_buffer);

/**
 * [THE COOPERATIVE TICK]
 * Called once per frame inside your main loop.
 * Reads exactly ONE physical CD sector (2048 bytes) per frame to guarantee
 * zero temporal drift or SH-4 frame spikes.
 */
void frog_loader_tick(frog_load_task_t *task);

/**
 * [MAINTENANCE]
 * Safely closes active descriptors and resets task values.
 */
void frog_loader_close(frog_load_task_t *task);

#endif
