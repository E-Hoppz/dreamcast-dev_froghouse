#ifndef HOST_COMPAT_DC_SOUND_SOUND_H
#define HOST_COMPAT_DC_SOUND_SOUND_H

#include <stdint.h>
#include <stdlib.h>

static inline int snd_init(void) { return 0; }

static inline uint32_t snd_mem_malloc(size_t size) { 
    return (uint32_t)((uintptr_t)malloc(size)); 
}

static inline int snd_sh4_to_aica(void *packet, size_t size) {
    (void)packet; (void)size;
    return 0;
}

static inline uint32_t snd_get_pos(uint32_t channel) {
    static uint32_t dummy_playhead = 0;
    // Simulates an advancing playhead based on regular frames
    dummy_playhead = (dummy_playhead + 735) % 65536;
    return dummy_playhead;
}

#endif
