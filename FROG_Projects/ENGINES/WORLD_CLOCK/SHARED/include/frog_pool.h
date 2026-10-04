#ifndef FROG_POOL_H
#define FROG_POOL_H

#include <stdint.h>

typedef enum {
    POOL_SYSTEM, 
    POOL_AICA,   
    POOL_VRAM    
} frog_pool_type_t;

typedef struct FrogBlock {
    struct FrogBlock *next;
} FrogBlock;

typedef struct {
    uintptr_t start_addr; 
    uint32_t size;
    uint32_t current_offset;
    frog_pool_type_t type;
    
    // Block-Pool Free-List Support
    uint32_t   block_size;
    FrogBlock *free_list;
} frog_pool_t;

void      frog_pool_init(frog_pool_t *pool, uint32_t size, frog_pool_type_t type);
uintptr_t frog_pool_alloc(frog_pool_t *pool, uint32_t size);
void      frog_pool_reset(frog_pool_t *pool);

void      frog_pool_init_block(frog_pool_t *pool, uint32_t block_size);
uintptr_t frog_pool_alloc_block(frog_pool_t *pool);
void      frog_pool_free_block(frog_pool_t *pool, uintptr_t ptr);

#endif
