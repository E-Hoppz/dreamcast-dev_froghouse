#include "frog_clockwork_bridge.h"

void frog_clockwork_inject(FrogClockworkTarget *target, WorldClock *wc) {
    if (!target || !wc) return;

    // 1. Core clockwork update (Updates AICA playhead status safely over G2)
    world_clock_update(wc);

    // 2. Inject SPU-slaved timing directly into target engine's address spaces
    if (target->engine_delta_time) {
        *(target->engine_delta_time) = (double)wc->delta_time;
    }
    if (target->engine_total_ticks) {
        *(target->engine_total_ticks) = (uint64_t)wc->audio.sample_count;
    }
}
