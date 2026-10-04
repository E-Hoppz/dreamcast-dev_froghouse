#ifndef WORLDCLOCK_FORALL_H
#define WORLDCLOCK_FORALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Opaque structure handle to encapsulate internal WorldClock state
typedef struct WorldClockOpaque WorldClockOpaque;

// Target pointers to tie into the host engine's timing parameters
typedef struct {
    double *engine_delta_time;   // Maps directly to host.frametime
    uint64_t *engine_total_ticks; // Maps to tick counter or frame accumulator
} WorldClockBridgeTarget;

// Engine Initialization & Configuration
WorldClockOpaque* world_clock_engine_init(void);
void world_clock_engine_shutdown(WorldClockOpaque *wc);

// Injector called once per frame inside Xash3D's main loop
void world_clock_engine_inject(WorldClockOpaque *wc, WorldClockBridgeTarget *target);

#ifdef __cplusplus
}
#endif

void world_clock_engine_draw_telemetry(WorldClockOpaque *wc, int fps);

#endif // WORLDCLOCK_FORALL_H
