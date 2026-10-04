//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
// Standalone sovereign game executive entry-point. Operates natively 
// on stock 16MB hardware bypassing external engines entirely.

#include <kos.h>
#include <dc/pvr.h>
#include <dc/video.h>
#include <stdio.h>
#include <stdbool.h>

#include "world_clock.h"
#include "frog_audio.h"
#include "frog_draw.h"
#include "frog_input.h"
#include "frog_physics.h"
#include "frog_cam.h"
#include "frog_mesh.h"
#include "frog_scratchpad.h"
#include "frog_config.h"
#include "frog_color.h"
#include "frog_entity.h"

// Link with the engine's master timing clock
extern WorldClock wc;

// Sample custom target rotation tracker for the standalone loop
static float g_rotation_angle = 0.0f;

// Basic sovereign shutdown routine
static void sovereign_shutdown() {
    frog_audio_stream_stop();
    pvr_shutdown();
    vid_shutdown();
    printf("SOVEREIGN: Engine sequence terminated gracefully.\n");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("FROGHOUSE: Starting Standalone World Clock Boot Sequence...\n");

    // 1. Core Hardware Initializations
    frog_config_init();
    frog_scratchpad_init();
    frog_input_init();
    frog_draw_init(); // Sets up PVR, frame sizes, and fog tables

    // 2. Set Up Color Pipelines
    frog_color_init();
    frog_color_set_pigment(PIGMENT_GLIDE_DAC);
    frog_engine_config.active_contract = CONTRACT_GLIDE;
    pvr_set_dither(true);

    // 3. Audio & Clock Activation
    frog_audio_init(); // Guarded within frog_audio to prevent double initialization
    world_clock_init(&wc);

    // 4. Spawn sovereign game entities and setup environment
    frog_entity_system_init();
    
    frog_cam_t main_cam;
    frog_cam_init(&main_cam, FROG_CAM_3D);
    main_cam.position = (vec3_t){ 0.0f, 15.0f, -40.0f, 1.0f };
    main_cam.target   = (vec3_t){ 0.0f, 0.0f, 0.0f, 1.0f };

    // Setup dummy target vector
    vec3_t actor_pos = {0.0f, 0.0f, 0.0f, 1.0f};

    printf("SOVEREIGN: System operational. Engaging unyielding 44.1kHz control loop.\n");

    // Primary Sovereign execution loop
    while(1) {
        // Clear frame-end Scratchpad bumps
        frog_scratchpad_reset();

        // Check Maple controllers and update system-level state variables
        frog_input_cache_dma();
        frog_input_update(&wc.input, &wc);

        // Break loop and return to console if START is held
        if (wc.input.start) {
            break;
        }

        // Slaves core universe time delta directly to AICA SPU playhead
        world_clock_update(&wc);

        // Advance Entity Logic and Script schedulers
        frog_entity_update_system(&wc);

        // Dynamic visual parameter tweaks based on telemetry input
        g_rotation_angle += 1.5f * wc.delta_time;
        if(g_rotation_angle > 6.28318f) g_rotation_angle -= 6.28318f;

        // Camera updates (including wave surface collision check)
        frog_cam_update(&main_cam, actor_pos, &wc);

        // Shunt standard frame structure pipeline to PowerVR Tile Accelerator
        pvr_wait_ready();
        pvr_scene_begin();

        // ---------------- OPAQUE SUBMISSION LIST ----------------
        pvr_list_begin(PVR_LIST_OP_POLY);
        frog_cam_apply(&main_cam);
        
        // Custom sovereign opaque models and grid drawing go here
        pvr_list_end();

        // ---------------- TRANSLUCENT SUBMISSION LIST ----------------
        pvr_list_begin(PVR_LIST_TR_POLY);
        
        // Sub-bass HUD visualizer rect
        float bass_meter_height = wc.sub_bass * 100.0f;
        frog_draw_rect_tr(30.0f, 30.0f, 15.0f, bass_meter_height, 0x9000FF00);

        // Debug diagnostic overlay text
        char debug_buf[64];
        sprintf(debug_buf, "WORLD CLOCK CLK: %u", (unsigned int)wc.audio.monotonic_samples);
        frog_draw_text(60.0f, 40.0f, debug_buf);
        
        pvr_list_end();

        // Complete the PowerVR polygon render phase
        pvr_scene_finish();
        
        // Cooperative sleep to allow kernel thread context switches
        thd_pass();
    }

    sovereign_shutdown();
    return 0;
}
