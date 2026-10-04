#include "frog_debug_live.h"
#include "frog_input.h" 
#include "frog_draw.h"   
#include <stdio.h>
#include <kos.h>         

static debug_param_t registry[32]; 
static int param_count = 0;
static int selected_idx = 0;
static bool debug_visible = false; // THE TOGGLE FLAG

void frog_debug_register_param(const char *label, float *val, float step) {
    if(param_count < 32) {
        registry[param_count].label = label;
        registry[param_count].value = val;
        registry[param_count].step = step;
        param_count++;
    }
}

void frog_debug_update_and_render(FrogInput *in) {
    if(in == NULL) return;

    // --- [TOGGLE LOGIC] ---
    // Start + Y toggles the menu visibility
    if(in->start && in->btn_y) {
        debug_visible = !debug_visible;
        thd_sleep(200); // Prevent rapid flickering
    }

    if(!debug_visible || param_count == 0) return;

    // --- [NAVA-STYLE INPUT LOGIC] ---
    if(in->dpad_up && selected_idx > 0) {
        selected_idx--;
        thd_sleep(150); 
    }
    
    if(in->dpad_down && selected_idx < param_count - 1) {
        selected_idx++;
        thd_sleep(150);
    }

    // Live Tweaking (D-pad Left/Right)
    if(in->dpad_left) {
        *registry[selected_idx].value -= registry[selected_idx].step;
        thd_sleep(50); 
    }
    if(in->dpad_right) {
        *registry[selected_idx].value += registry[selected_idx].step;
        thd_sleep(50);
    }

    // --- [VISUAL RENDERING] ---
    // Transparent Black background
    frog_draw_rect(20, 40, 280, 20 + (param_count * 20) + 10, 0x90000000); 

    char buffer[64];
    for(int i = 0; i < param_count; i++) {
        // Highlighting the selection
        if(i == selected_idx) {
            sprintf(buffer, " > %s: %.2f", registry[i].label, *registry[i].value);
        } else {
            sprintf(buffer, "   %s: %.2f", registry[i].label, *registry[i].value);
        }
        
        frog_draw_text(30, 50 + (i * 20), buffer);
    }
}
