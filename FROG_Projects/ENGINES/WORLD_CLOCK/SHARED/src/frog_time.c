#include "frog_time.h"

void frog_timer_start(FrogTimer *t) {
    t->start_time = timer_ms_gettime64();
    t->is_running = 1;
}

float frog_timer_get_secs(FrogTimer *t) {
    if (!t->is_running) return 0.0f;
    uint64 now = timer_ms_gettime64();
    return (float)(now - t->start_time) / 1000.0f;
}
