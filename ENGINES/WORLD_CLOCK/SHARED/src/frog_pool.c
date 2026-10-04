#include "frog_pool.h"
#include "frog_util.h"
#include <dc/sound/sound.h> 
#include <dc/pvr.h>         
#include <stdlib.h>
#include <stdio.h>

#define FROG_ALIGN 32
#define ALIGN_VAL(x) (((x) + (FROG_ALIGN - 1)) & ~(FROG_ALIGN - 1))

void frog_pool_init(frog_pool_t *pool, uint32_t size, frog_pool_type_t type) {
    pool->size = size;
    pool->current_offset = 0;
    pool->type = type;
    pool->block_size = 0;
    pool->free_list = NULL;

    switch(type) {
        case POOL_SYSTEM:
            pool->start_addr = (uintptr_t)malloc(size);
            break;
        case POOL_AICA:
            pool->start_addr = (uintptr_t)snd_mem_malloc(size);
            break;
        case POOL_VRAM:
            pool->start_addr = (uintptr_t)pvr_mem_malloc(size);
            break;
    }

    if (!pool->start_addr) {
        printf("POOL CRITICAL FAILURE: Could not open Memory Rail Type %d\n", type);
    } else {
        frog_util_log("POOL: Memory Arena calibrated and online.");
    }
}

uintptr_t frog_pool_alloc(frog_pool_t *pool, uint32_t size) {
    uint32_t aligned_size = ALIGN_VAL(size);

    if (pool->current_offset + aligned_size > pool->size) {
        frog_util_log("POOL CRITICAL: Arena full! Signal lost.");
        return 0;
    }

    uintptr_t addr = pool->start_addr + pool->current_offset;
    pool->current_offset += aligned_size;

    return addr;
}

void frog_pool_reset(frog_pool_t *pool) {
    pool->current_offset = 0;
    pool->free_list = NULL;
    
    if(pool->type == POOL_SYSTEM && pool->start_addr != 0) {
        frog_util_memzero((void*)pool->start_addr, pool->size);
    }

    frog_util_log("POOL: Arena purged. Resetting...");
}

void frog_pool_init_block(frog_pool_t *pool, uint32_t block_size) {
    pool->block_size = ALIGN_VAL(block_size);
    pool->free_list = NULL;
}

uintptr_t frog_pool_alloc_block(frog_pool_t *pool) {
    if (!pool || pool->block_size == 0) return 0;

    // Pull from free-list if available
    if (pool->free_list != NULL) {
        uintptr_t addr = (uintptr_t)pool->free_list;
        pool->free_list = pool->free_list->next;
        return addr;
    }

    // Fall back to arena bump-allocation
    return frog_pool_alloc(pool, pool->block_size);
}

void frog_pool_free_block(frog_pool_t *pool, uintptr_t ptr) {
    if (!pool || ptr == 0) return;

    FrogBlock *block = (FrogBlock *)ptr;
    block->next = pool->free_list;
    pool->free_list = block;
}
