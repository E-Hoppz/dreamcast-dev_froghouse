#ifndef FROG_DEBUG_LIVE_H
#define FROG_DEBUG_LIVE_H

#include "frog_input.h"

#define MAX_DEBUG_PARAMS 32

typedef struct {
    const char *label;
    float *value;
    float step;
} debug_param_t;

// The "Contract" for the Lair
void frog_debug_register_param(const char *label, float *val, float step);
void frog_debug_update_and_render(FrogInput *in);

#endif
