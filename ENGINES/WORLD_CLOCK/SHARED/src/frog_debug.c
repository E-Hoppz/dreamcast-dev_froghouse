//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
// Optimized diagnostic router. Redirects telemetry to standard serial/console output
// to prevent memory-mapping protection exceptions during active PVR/GLdc sessions.

#include "frog_debug.h"
#include <dc/video.h>
#include <dc/biosfont.h>
#include <stdio.h>

void frog_debug_clear_screen() {
    // Bypassed: Direct clearing is skipped during active hardware rendering
}

void frog_debug_draw_text(int x, int y, const char *str) {
    // Safely route telemetry text to standard console standard out
    printf("[WORLD CLOCK DEBUG] (%d, %d): %s\n", x, y, str);
}

void frog_debug_draw_rect(int x1, int y1, int x2, int y2, uint32_t color) {
    // Bypassed: Raw VRAM writes are skipped to prevent MMU protection faults
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
}

void frog_debug_draw_box(int x1, int y1, int x2, int y2, uint32_t color) {
    // Bypassed: Raw VRAM writes are skipped to prevent MMU protection faults
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
}
