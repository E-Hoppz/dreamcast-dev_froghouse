#ifndef HOST_COMPAT_DC_SPU_H
#define HOST_COMPAT_DC_SPU_H

#include <stdint.h>
#include <string.h>

static inline void spu_memload(uint32_t dst, const void *src, size_t size) {
    (void)dst; (void)src; (void)size; // Mock sink
}

#endif
