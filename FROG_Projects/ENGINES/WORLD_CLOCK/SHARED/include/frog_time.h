#ifndef FROG_TIME_H
#define FROG_TIME_H

#include <arch/types.h>
#include <arch/timer.h>

typedef struct {
    uint64 start_time;
    uint64 paused_time;
    int    is_running;
} FrogTimer;

// Starts or Resets the timer
void frog_timer_start(FrogTimer *t);

// Returns the elapsed seconds as a float (e.g. 1.5 seconds)
float frog_timer_get_secs(FrogTimer *t);

#endif
