#include <kos.h>
#include "vehicle.h"      
#include "controls.h"     
#include "world_clock.h"  
#include "frog_math.h"    
#include "frog_math_3d.h" // [NEW 1] 3D Vector DNA
#include "frog_input.h"   
#include "frog_debug.h"   
#include "frog_debug_live.h" // [NEW 2] The Lair Live Editor
#include "frog_physics.h" 
#include "frog_config.h"  
#include "frog_stats.h"   
#include "frog_util.h"    
#include "frog_time.h"

/* [NOTE 1] GLOBAL STATE */
VehicleState player_car = { 800.0f, 0.0f, 0, 0.0f, 0.0f, 0.0f, 0 };
FrogConfig    game_cfg;   
FrogInput     frog_in;    
AudioClock    master_clk = { 44100, 0, 1.0f }; 

/* [NEW 3] LIVE PARAMETERS */
// These variables can now be changed WHILE THE GAME IS RUNNING
float gravity_val = 9.8f;
float grip_multiplier = 1.0f;
int selected_debug_idx = 0;

debug_param_t lair_menu[] = {
    {"Redline", &game_cfg.engine_redline, 100.0f},
    {"Gravity", &gravity_val, 0.1f},
    {"Grip", &grip_multiplier, 0.05f}
};

int main(int argc, char **argv) {
    printf("IHPSO: Island Heat Psycho Shift Overdrive - BOOTING\n");

    /* [NOTE 2] CONFIG LOADING */
    if(!frog_load_config("/rd/game.cfg", &game_cfg)) {
        game_cfg.engine_redline = 8000.0f; 
    }

    while(1) {
        cont_state_t *raw = get_elisha_input(); 
        if(raw) { 
            // 1. SYNC RAW INPUT
            frog_map_input(raw, &frog_in);

            /* [NEW 4] LIVE MENU NAVIGATION */
            // Using D-Pad to tune the engine in real-time
            if(raw->buttons & CONT_DPAD_UP) selected_debug_idx = (selected_debug_idx > 0) ? selected_debug_idx - 1 : 2;
            if(raw->buttons & CONT_DPAD_DOWN) selected_debug_idx = (selected_debug_idx < 2) ? selected_debug_idx + 1 : 0;
            if(raw->buttons & CONT_DPAD_LEFT) *lair_menu[selected_debug_idx].value -= lair_menu[selected_debug_idx].step;
            if(raw->buttons & CONT_DPAD_RIGHT) *lair_menu[selected_debug_idx].value += lair_menu[selected_debug_idx].step;

            // 2. MAP FROG ACTIONS TO VEHICLE
            player_car.throttle_pos = frog_in.throttle;
            player_car.brake_pressure = frog_in.brake;

            /* [NOTE 3] YOUR ORIGINAL STALL LOGIC */
            if(!player_car.is_stalled) {
                if(player_car.throttle_pos > 0.1f) {
                    player_car.rpm += (150.0f * player_car.throttle_pos);
                } else {
                    player_car.rpm -= 40.0f; 
                }

                if(player_car.clutch_pos < 0.1f && player_car.velocity == 0.0f && player_car.throttle_pos < 0.1f) {
                    player_car.is_stalled = 1;
                    player_car.rpm = 0.0f;
                }

                /* [NOTE 4] NEW REV LIMITER BOUNCE */
                // Now uses the LIVE-TUNED redline value!
                if(player_car.rpm >= game_cfg.engine_redline) {
                    player_car.rpm = game_cfg.engine_redline - 400.0f; 
                }
                
                player_car.rpm = frog_clamp(player_car.rpm, 800.0f, game_cfg.engine_redline);

            } else { 
                /* YOUR ORIGINAL RESTART LOGIC */
                if(player_car.gear == 0 && (raw->buttons & CONT_START)) {
                    player_car.is_stalled = 0;
                    player_car.rpm = 800.0f;
                }
            }

            /* [NOTE 5] SYNC THE HEARTBEAT */
            master_clk.pitch_scale = player_car.rpm / game_cfg.engine_redline; 
            sync_to_aica(&master_clk); 

            /* [NEW 5] RENDER THE LIVE EDITOR */
            // This draws the menu over your game
            frog_debug_render_menu(lair_menu, 3, selected_debug_idx);
        }

        thd_sleep(10); 
    }
    return 0;
}
