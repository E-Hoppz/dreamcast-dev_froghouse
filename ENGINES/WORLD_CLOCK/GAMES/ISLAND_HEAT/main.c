#include <kos.h>
#include <stdio.h>
#include <stdlib.h>   // Required for standard malloc/free/calloc
#include <math.h>
#include <unistd.h>   // [REPAIRMAN'S FIX] Required for isatty/close/read/write/lseek
#include <sys/stat.h> // [REPAIRMAN'S FIX] Required for fstat / struct stat
#include <dc/fmath.h> // For hardware-accelerated fsin/fcos
#include <kos/dbgio.h> // [REPAIRMAN'S FIX] Standard hardware debugging console I/O
#include "world_clock.h"
#include "frog_draw.h"
#include "frog_audio.h"
#include "frog_input.h"
#include "frog_color.h"
#include "frog_math_3d.h"
#include "frog_physics.h"
#include "frog_cam.h"
#include "frog_util.h" // Include to expose frog_util_log

// [REPAIRMAN'S REGULATOR]
// Physics runs at 240Hz (Sub-frame precision)
#define PHYSICS_STEPS_PER_SEC 240
#define SAMPLES_PER_STEP (44100 / PHYSICS_STEPS_PER_SEC)

// [THE STATE MACHINE]
typedef enum {
    FROG_STATE_PLAYING,
    FROG_STATE_PAUSED,
    FROG_STATE_FINISHED 
} EngineState;

EngineState current_state = FROG_STATE_PLAYING;

// [THE PROJECT BENCH]
WorldClock wc;
frog_cam_t viewport;
FrogBody   cube; 
FrogPhysicsConstants cube_specs = { 1.0f, 0.1f, 0.05f, 0.8f, 2.5f };

// Global Camera/Timing Signals
float cam_angle = 0.0f;
float cam_pitch = 0.5f;
uint32_t accumulator = 0;
const char* resolved_audio_path = NULL;

// Debouncing buffers (Ensures edge-triggered button presses)
bool prev_btn_a = false;
bool prev_btn_b = false;
bool prev_start = false;

// ================================================================
// GCC 15 + NEWLIB 4.6.0 COMPATIBILITY LAYER & REENTRANCY GLUE
// Bridges the gap between modern Newlib stdio, KallistiOS, and older ldscripts.
// Maps reentrant system calls directly to native KOS allocators/handlers.
// ================================================================

// [REPAIRMAN'S FIX] GCC 15 TLS Absolute Symbol Mapping
// We allocate a safe 1024-byte buffer for the Thread Local Storage.
// This ensures Newlib's thread-local variables fall safely inside a valid,
// allocated RAM block, preventing the SH-4 write access exception on startup.
asm(".global _tdata_size\n.set _tdata_size, 1024");
asm(".global _tbss_size\n.set _tbss_size, 1024");
asm(".global _tdata_align\n.set _tdata_align, 4");
asm(".global _tbss_align\n.set _tbss_align, 4");

// Expose assembly symbols to the C compiler so it can parse keep-references
extern uint32_t _tdata_size;
extern uint32_t _tbss_size;
extern uint32_t _tdata_align;
extern uint32_t _tbss_align;

// Matches the exact void pointer type declared in KOS's init.h
const void * __kos_romdisk = NULL;

// Non-recursive I/O stubs to prevent stack overflow loops.
int _close_r(struct _reent *r, int fd) {
    (void)r; (void)fd;
    return 0;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t cnt) {
    (void)r; (void)fd; (void)buf; (void)cnt;
    return 0; // Return EOF
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t cnt) {
    (void)r; (void)fd; (void)buf;
    // Safe dummy return to satisfy Newlib's stream initializers
    // during early boot without invoking uninitialized hardware/dbgio layers.
    return cnt; 
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t offset, int whence) {
    (void)r; (void)fd; (void)offset; (void)whence;
    return 0;
}
// ================================================================

// Locates the song across multiple mounting points
const char* resolve_audio_path() {
    file_t f;

    // 1. Check physical CD-ROM track (burnt CD or Emulator ISO mount)
    f = fs_open("/cd/mixtape.raw", O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return "/cd/mixtape.raw";
    }

    // 2. Fallback: Host PC directory (testing via dc-tool-ip or dc-tool-ser)
    f = fs_open("/pc/mixtape.raw", O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return "/pc/mixtape.raw";
    }

    // 3. Fallback: Virtual romdisk directory
    f = fs_open("/rd/mixtape.raw", O_RDONLY);
    if (f != FILEHND_INVALID) {
        fs_close(f);
        return "/rd/mixtape.raw";
    }

    return NULL;
}

// Resets objects back to their default structural states
void restart_prototype() {
    cube.position = (vec3_t){0.0f, 50.0f, 0.0f, 1.0f};
    cube.velocity = (vec3_t){0.0f, 0.0f, 0.0f, 0.0f};
    cube.acceleration = (vec3_t){0.0f, 0.0f, 0.0f, 0.0f};
    
    if (resolved_audio_path) {
        frog_audio_stream_start(resolved_audio_path);
    }
    
    wc.audio.is_done = false;
    wc.audio.is_paused = false;
    accumulator = 0;
    current_state = FROG_STATE_PLAYING;
}

int main(int argc, char **argv) {
    // [REPAIRMAN'S SHUNT] Force value reads to our compatibility layer variables
    // inside the main entry point to protect them from the linker's garbage collector.
    {
        volatile uint32_t keep_tdata = _tdata_align;
        volatile uint32_t keep_tbss = _tbss_align;
        volatile uint32_t keep_tdsz = _tdata_size;
        volatile uint32_t keep_tbsz = _tbss_size;
        volatile const void *keep_rom = __kos_romdisk;

        (void)keep_tdata; (void)keep_tbss; (void)keep_tdsz; (void)keep_tbsz; (void)keep_rom;
    }

    // 1. BOOT SEQUENCE
    frog_audio_init();   
    frog_draw_init();    
    world_clock_init(&wc);
    frog_physics_init();
    frog_color_init(); 
    
    // Set Vibe to Golden Hour Nemo Glow
    frog_color_set_params(1.3f, 1.5f);
    frog_color_set_pigment(PIGMENT_NEMO);

    // Initial Hardware Calibration
    cube.position = (vec3_t){0.0f, 50.0f, 0.0f, 1.0f};
    frog_cam_init(&viewport, FROG_CAM_3D);
    
    // Auto-detect best file system mount
    resolved_audio_path = resolve_audio_path();
    if (resolved_audio_path) {
        frog_audio_stream_start(resolved_audio_path);
    } else {
        frog_util_log("WARNING: mixtape.raw could not be resolved on any drive mount.");
    }

    while(1) {
        // 2. CAPTURE MECHANICAL TRUTH
        uint32_t last_sample = wc.audio.sample_count;
        world_clock_update(&wc); 
        
        // Edge check: transition state if audio playhead signals end of track
        if (wc.audio.is_done && current_state != FROG_STATE_FINISHED) {
            frog_audio_stream_stop();
            current_state = FROG_STATE_FINISHED;
        }

        uint32_t samples_passed = wc.audio.sample_count - last_sample;
        
        // Feed the clock accumulator only when simulation is unpaused
        if (current_state == FROG_STATE_PLAYING) {
            accumulator += samples_passed;
        }

        // 3. THE HIGH-FREQUENCY LOGIC RAIL (240Hz Loop)
        while (accumulator >= SAMPLES_PER_STEP) {
            frog_input_update(&wc.input, &wc);

            // Debounced Global Restart (START Button)
            if (wc.input.start && !prev_start) {
                restart_prototype();
            }

            if (current_state == FROG_STATE_PLAYING) {
                // Debounced Pausing (B Button)
                if (wc.input.btn_b && !prev_btn_b) {
                    frog_audio_stream_pause();
                    current_state = FROG_STATE_PAUSED;
                }

                // Camera Orbit Control (Hold Y + Stick)
                if (wc.input.btn_y) {
                    cam_angle += wc.input.stick_x * 0.02f;
                    cam_pitch += wc.input.stick_y * 0.02f;
                    if (cam_pitch < 0.1f) cam_pitch = 0.1f;
                    if (cam_pitch > 1.4f) cam_pitch = 1.4f;
                }

                // Orbital Math (Stay between Cube and Sun)
                float radius = 200.0f;
                vec3_t target_cam;
                target_cam.x = cube.position.x + fcos(cam_angle) * radius;
                target_cam.z = cube.position.z + fsin(cam_angle) * radius;
                target_cam.y = cube.position.y + (cam_pitch * 150.0f);
                target_cam.w = 1.0f;
                frog_cam_update(&viewport, target_cam, &wc);

                // Physics: Cube Bobbing (Resonate with Sub-Bass)
                float water_h = frog_physics_get_wave_height(cube.position.x, cube.position.z, &wc);
                if (cube.position.y < water_h) {
                    cube.acceleration.y += cube_specs.buoyancy;
                }
                frog_physics_update(&cube, &cube_specs, SAMPLES_PER_STEP);
            }
            else if (current_state == FROG_STATE_PAUSED) {
                // Debounced Unpausing (B Button)
                if (wc.input.btn_b && !prev_btn_b) {
                    frog_audio_stream_pause();
                    current_state = FROG_STATE_PLAYING;
                }

                // Orbit camera is kept fully active while paused to allow inspect modes
                if (wc.input.btn_y) {
                    cam_angle += wc.input.stick_x * 0.02f;
                    cam_pitch += wc.input.stick_y * 0.02f;
                    if (cam_pitch < 0.1f) cam_pitch = 0.1f;
                    if (cam_pitch > 1.4f) cam_pitch = 1.4f;
                }
                float radius = 200.0f;
                vec3_t target_cam;
                target_cam.x = cube.position.x + fcos(cam_angle) * radius;
                target_cam.z = cube.position.z + fsin(cam_angle) * radius;
                target_cam.y = cube.position.y + (cam_pitch * 150.0f);
                target_cam.w = 1.0f;
                frog_cam_update(&viewport, target_cam, &wc);
            }
            else if (current_state == FROG_STATE_FINISHED) {
                // Debounced Replay (A Button)
                if (wc.input.btn_a && !prev_btn_a) {
                    restart_prototype();
                }
            }

            // Lock previous frame inputs for the next logic iteration
            prev_btn_a = wc.input.btn_a;
            prev_btn_b = wc.input.btn_b;
            prev_start = wc.input.start;

            accumulator -= SAMPLES_PER_STEP;
        }

        // 4. THE VIDEO RAIL (Mano de Oro Standard)
        pvr_wait_ready();
        pvr_scene_begin();
            
            pvr_list_begin(PVR_LIST_OP_POLY);
                frog_draw_rect(0, 0, 640, 480, 0xFF000005); // Lair Black-Blue

                // THE SUN (Scales with Treble)
                float sun_sz = 30.0f + (wc.treble * 30.0f);
                frog_draw_rect(400, 80, sun_sz, sun_sz, 0xFFFFFFCC);

                // DYNAMIC WATER (Layered Signal)
                for(int i=0; i<4; i++) {
                    float h = frog_physics_get_wave_height(0, (float)i * 10.0f, &wc);
                    uint32_t w_col = (wc.sub_bass > 0.5f) ? 0xFF00FFFF : 0xFF004455;
                    frog_draw_rect(0, 320 + (i*12) + h, 640, 2, w_col);
                }
            pvr_list_finish();

            pvr_list_begin(PVR_LIST_TR_POLY);
                // Apply metallic color transforms on target coordinate
                uint32_t gold_vibe = frog_color_apply_vibe(0xFFFFD700, 100.0f, &wc);
                
                // Scale positions
                float sx = 320.0f; 
                float sy = 240.0f - cube.position.y;
                float sz = 100.0f + (wc.sub_bass * 40.0f);

                // BASE METAL PASS
                frog_draw_rect_tr(sx-(sz/2), sy-(sz/2), sz, sz, gold_vibe);
                
                // SPECULAR NEMO POP (Additive inner core)
                float pop_sz = sz * 0.6f;
                uint32_t specular_color = PVR_PACK_COLOR(0.5f, wc.mids, wc.treble, 0.5f);
                frog_draw_rect_tr(sx-(pop_sz/2), sy-(pop_sz/2), pop_sz, pop_sz, specular_color);

                // UNDERWATER LIGHT
                if (wc.sub_bass > 0.4f) {
                    frog_draw_rect_tr(sx-100, 360, 200, 40, 0x4400FFFF);
                }
            pvr_list_finish();
            
        pvr_scene_finish();

        // 5. HUD TELEMETRY VISUALS
        if (current_state == FROG_STATE_FINISHED) {
            // Replay screen overlay
            frog_draw_rect_tr(100, 180, 440, 120, 0xD0000000); 
            frog_draw_text(160, 210, "TRACK COMPLETION REACHED");
            frog_draw_text(190, 250, "PLAY AGAIN? PRESS [A]");
        } else {
            char hud[128];
            sprintf(hud, "STICK: %.2f, %.2f | TRG: %.2f, %.2f", 
                    wc.input.stick_x, wc.input.stick_y, wc.input.axis_l, wc.input.axis_r);
            frog_draw_text(20, 410, hud);
            
            sprintf(hud, "SUB: %.2f | MID: %.2f | TREB: %.2f", wc.sub_bass, wc.mids, wc.treble);
            frog_draw_text(20, 430, hud);

            if (current_state == FROG_STATE_PAUSED) {
                sprintf(hud, "WORLD CLOCK: PAUSED (ORBIT ENGAGED)");
            } else {
                sprintf(hud, "WORLD CLOCK: 44.1kHz [RES: 240Hz]");
            }
            frog_draw_text(20, 450, hud);
        }
    }

    return 0;
}
