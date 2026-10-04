#ifndef FROG_DEBUG_H
#define FROG_DEBUG_H

#include <stdint.h>

/**
 * [THE DEBUG ENGINE COUPLING]
 * Direct-to-VRAM diagnostic renderers.
 * Use these wrapper routines to display real-time values bypassing the PVR pipeline.
 */
void frog_debug_draw_text(int x, int y, const char *str);
void frog_debug_clear_screen();
void frog_debug_draw_rect(int x1, int y1, int x2, int y2, uint32_t color);
void frog_debug_draw_box(int x1, int y1, int x2, int y2, uint32_t color);

#endif
