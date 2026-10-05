#ifndef HOST_COMPAT_DC_BIOSFONT_H
#define HOST_COMPAT_DC_BIOSFONT_H

#include <stdint.h>

static inline void bfont_draw_str(uint16_t *vram, int width, int opaque, const char *str) {
    (void)vram; (void)width; (void)opaque; (void)str;
}

#endif
