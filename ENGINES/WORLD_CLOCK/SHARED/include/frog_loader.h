#ifndef FROG_LOADER_H
#define FROG_LOADER_H

#include <kos.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOAD_IDLE,      
    LOAD_PENDING,   
    LOAD_COMPLETE,  
    LOAD_ERROR      
} frog_load_status_t;

typedef struct {
    const char *filename;   
    void       *buffer;
    uint32_t    size;
    uint32_t    bytes_done;
    float       progress;
    
    frog_load_status_t status;
    file_t fd;
} frog_load_task_t;

extern volatile bool g_g2_bus_locked;

bool frog_loader_is_g2_locked(void);
bool frog_loader_start(frog_load_task_t *task, const char *path, void *target_buffer);
void frog_loader_tick(frog_load_task_t *task);
void frog_loader_close(frog_load_task_t *task);

#ifdef __cplusplus
}
#endif

#endif
