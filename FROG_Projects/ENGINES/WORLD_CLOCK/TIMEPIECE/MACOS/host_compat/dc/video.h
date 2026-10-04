#ifndef HOST_COMPAT_DC_VIDEO_H
#define HOST_COMPAT_DC_VIDEO_H

#include <stdint.h>

// Mocks the physical 16-bit 640x480 video RAM buffer
extern uint16_t host_vram_s[640 * 480];
#define vram_s host_vram_s

static inline void vid_clear(int r, int g, int b) {
    (void)r; (void)g; (void)b;
}

#endif
