#ifndef FROG_AIR_SIGNAL_H
#define FROG_AIR_SIGNAL_H

typedef struct {
    float lp_acc; // Low-pass capacitor
    float hp_acc; // High-pass capacitor
    float lp_coeff; // Low-pass (Sub-Bass) coefficient (e.g. 0.05f)
    float hp_coeff; // High-pass (Treble) coefficient (e.g. 0.40f)
} FrogAirCrossover;

typedef struct {
    float rez, frequency, position;
} FrogAirDeRez;

typedef struct {
    float mu_variable, stiffness, current_mass;
} FrogAirPressure;

typedef struct {
    float iir_acc;
} FrogAirDensity;

float frog_air_step_iir(float *accumulator, float input, float amount);
void  frog_air_crossover_init(FrogAirCrossover *state, float low_coeff, float high_coeff);
void  frog_air_isolate_bands(FrogAirCrossover *state, float input, float *low, float *mid, float *high);
float frog_air_step_derez(FrogAirDeRez *state, float input);
float frog_air_step_pressure(FrogAirPressure *state, float input);
float frog_air_step_spiral(float input);
float frog_air_step_density(FrogAirDensity *state, float input);

// --- [ NEW AIRWINDOWS PLUGINS ] ---
float frog_air_step_drive(float input, float gain);
float frog_air_console_encode(float input);
float frog_air_console_decode(float input);

#endif
