#ifndef FROG_INPUT_H
#define FROG_INPUT_H

#include <stdint.h>
#include <stdbool.h>

struct WorldClock;

typedef struct {
    uint32_t timestamp;

    // Digital Rail
    int dpad_up, dpad_down, dpad_left, dpad_right;
    int btn_a, btn_b, btn_x, btn_y;
    int btn_z, btn_c, btn_d; 
    int start;

    // Analog Signal
    float stick_x, stick_y;   
    float stick_r_x, stick_r_y; 
    float axis_l, axis_r;     
    float throttle;           

    // Lightgun / Mouse
    bool  gun_triggered;
    int   gun_x, gun_y;        
    float mouse_dx, mouse_dy; 
    int   mouse_buttons;

    // Hardware Flags
    int is_gun;         
    int is_mouse;       
    int is_kb;          
    int is_twinstick;   
    int is_wheel;       
    int is_ddr;         

    // Unified Actions
    bool action_primary;   
    bool action_secondary; 
    bool action_special;   

    bool mic_active;
} FrogInput;

// Power-on configuration and initialization
void frog_input_init(void);

void frog_input_update(FrogInput *frog, struct WorldClock *wc);

// Asynchronous Maple-DMA Frame Buffer caching trigger
void frog_input_cache_dma(void);

#endif
