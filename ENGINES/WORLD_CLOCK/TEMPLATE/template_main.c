#include <kos.h>
#include "world_clock.h"
#include "frog_audio.h"
#include "frog_physics.h"
#include "frog_input.h"
#include "frog_debug_live.h"

// [THE LAIR] Global State
WorldClock   wc;
FrogBody     player;
FrogPhysicsConstants phys_cfg = { 1.0f, 0.05f, 0.1f, 0.8f }; // Heavy Duty specs

int main(int argc, char **argv) {
    // 1. HARDWARE BOOT
    frog_audio_init();       // Slaves AICA to World Clock
    frog_physics_init();     // Tethers bodies to the AICA crystal
    world_clock_init(&wc);   // Starts the Master Heartbeat

    // 2. REGISTER "LAIR" TWEAKS (For live tuning on your CRT)
    frog_debug_register_param("Gravity", &phys_cfg.elasticity, 0.05f);
    frog_debug_register_param("Wave Amp", &phys_cfg.mass, 0.1f); // Using mass as a placeholder

    frog_util_log("FROGHOUSE ENGINE: Resonating at 44100Hz...");

    while(1) {
        // --- START OF WORLD CLOCK STEP ---
        
        // A. Capture Mechanical Truth
        uint32_t start_vblank = timer_ns_gettime64();
        world_clock_update(&wc); 
        frog_input_update(&wc.input);

        // B. Offload Wave Math (SURFING / JET SKI LOGIC)
        // We don't calculate waves on CPU. We peek at AICA Channel 60.
        float current_swell = frog_physics_get_wave_height(player.position.x, player.position.z, wc.audio.sample_count);
        
        // C. Genre-Specific Physics Shunt
        // If we are in a Racing/Surfing state, apply displacement
        if (player.position.y < current_swell) {
            player.acceleration.y += 2.0f; // Buoyancy pulse
        }

        // D. The Physics Step (Surgical 44.1kHz Delta)
        // world_clock_update calculated exactly how many audio samples passed
        frog_physics_update(&player, &phys_cfg, wc.audio.sample_count - wc.audio.current_offset);

        // E. Visual Resonance (Mano de Oro Glow)
        pvr_wait_ready();
        pvr_scene_begin();
            // Draw your character based on the World Clock position
            // Draw the "Lair" UI over the top
            frog_debug_update_and_render(&wc.input);
        pvr_scene_finish();

        // --- END OF WORLD CLOCK STEP ---
    }

    return 0;
}
