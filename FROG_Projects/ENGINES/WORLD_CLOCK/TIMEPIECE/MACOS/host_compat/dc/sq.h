#ifndef HOST_COMPAT_DC_SQ_H
#define HOST_COMPAT_DC_SQ_H

#include <string.h>

// Shims the 32-byte physical burst copy using standard memcpy
static inline void* sq_cpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

// Shims the 32-byte physical burst clear using standard memset
static inline void sq_clr(void *dest, size_t n) {
    memset(dest, 0, n);
}

#endif
