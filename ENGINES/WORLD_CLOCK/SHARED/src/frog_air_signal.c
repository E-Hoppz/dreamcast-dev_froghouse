#include "frog_air_signal.h"
#include <math.h>

float frog_air_step_iir(float *accumulator, float input, float amount) {
    *accumulator = (*accumulator * (1.0f - amount)) + (input * amount);
    return *accumulator;
}

// [REPAIRMAN'S SHUNT] Crossover dynamic coefficient initialization
void frog_air_crossover_init(FrogAirCrossover *state, float low_coeff, float high_coeff) {
    if (!state) return;
    state->lp_acc = 0.0f;
    state->hp_acc = 0.0f;
    state->lp_coeff = low_coeff;
    state->hp_coeff = high_coeff;
}

// [REPAIRMAN'S SHUNT] Multi-band frequency splitting slaved to dynamic coefficients
void frog_air_isolate_bands(FrogAirCrossover *state, float input, float *low, float *mid, float *high) {
    if (!state || !low || !mid || !high) return;

    // 1. Extract Sub-Bass using dynamic low-pass coefficient
    *low = frog_air_step_iir(&state->lp_acc, input, state->lp_coeff);

    // 2. Extract Treble using dynamic high-pass coefficient
    float hp_signal = frog_air_step_iir(&state->hp_acc, input, state->hp_coeff);
    *high = input - hp_signal;

    // 3. Extract Mids (the remaining signal voltage)
    *mid = input - (*low + *high);
}

float frog_air_step_derez(FrogAirDeRez *state, float input) {
    float target = input;
    
    state->position += state->frequency;
    if (state->position > 1.0f) {
        state->position -= 1.0f;
        
        float sense = state->rez * 32768.0f;
        target = floorf(input * sense) / sense;
    }
    return target;
}

float frog_air_step_pressure(FrogAirPressure *state, float input) {
    float pre_gain = 2.0f; 
    
    float squash = fabsf(input) * pre_gain;
    if (squash > 1.0f) squash = 1.0f;
    
    return sinf(input * (1.0f - squash));
}

float frog_air_step_spiral(float input) {
    if (input > 1.2533141373155f) input = 1.2533141373155f;
    if (input < -1.2533141373155f) input = -1.2533141373155f;
    
    return sinf(input * fabsf(input));
}

float frog_air_step_density(FrogAirDensity *state, float input) {
    float density = 1.5f;
    float out = sinf(input * density);
    
    return frog_air_step_iir(&state->iir_acc, out, 0.1f);
}

// --- [ NEW AIRWINDOWS IMPLEMENTATIONS ] ---

float frog_air_step_drive(float input, float gain) {
    float temp = input * gain;
    if (temp > 1.0f)  temp = 1.0f;
    if (temp < -1.0f) temp = -1.0f;
    
    // Sinusoidal analog saturation curve
    return sinf(temp * 1.57079632679f);
}

float frog_air_console_encode(float input) {
    // Squeezes coordinate/signal peaks (compression)
    return sinf(input);
}

float frog_air_console_decode(float input) {
    // Expands coordinate/signal peaks (expansion)
    return asinf(input);
}
