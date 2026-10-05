#ifndef HOST_COMPAT_ARCH_TIMER_H
#define HOST_COMPAT_ARCH_TIMER_H

#include <stdint.h>
#include <time.h>

static inline uint64_t timer_ms_gettime64(void) {
#if defined(_WIN32)
    return (uint64_t)clock() * 1000 / CLOCKS_PER_SEC;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

#endif
