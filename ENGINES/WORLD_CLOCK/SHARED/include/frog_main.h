#ifndef FROG_MAIN_H
#define FROG_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * [THE SOVEREIGN INITIALIZATION]
 * Configures the bare-metal hardware subsystems:
 * - Directs the PowerVR2 Tile Accelerator memory allocations.
 * - Splits the L1 Data Cache to configure 4KB Scratchpad RAM (0x7c000000).
 * - Safe SPU sound driver and MIDI stream initialization.
 * - Masking the SH-4 FPU trap registers to avoid denormalized division freezes.
 */
void frog_sovereign_init(void);

/**
 * [THE UNYIELDING TIMING LOOP]
 * Executes the core 60Hz game loop, updating inputs, the SPU-slaved World Clock,
 * 3D scene hierarchies, and flushing geometry passes to the tiler.
 */
void frog_sovereign_loop(void);

/**
 * [THE HARDWARE POWER OFF]
 * Safe disassembly of SPU pipelines and standard video buffer closures.
 */
void frog_sovereign_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // FROG_MAIN_H
