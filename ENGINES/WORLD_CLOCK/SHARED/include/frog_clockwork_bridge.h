#ifndef FROG_CLOCKWORK_BRIDGE_H
#define FROG_CLOCKWORK_BRIDGE_H

#include "world_clock.h"

// Universal structure representing any engine's time/delta matrix
typedef struct {
    double *engine_delta_time;   // Pointer to target engine's float/double dt variable
    uint64_t *engine_total_ticks; // Pointer to target engine's total ticks/frames variable
} FrogClockworkTarget;

/**
 * [THE CLOCKWORK COUPLING]
 * Universal injector. Overwrites any target engine's internal time delta
 * (such as GoldSource's host_frametime or Quake's sys_tic) with the SPU-slaved World Clock.
 */
void frog_clockwork_inject(FrogClockworkTarget *target, WorldClock *wc);

#endif
