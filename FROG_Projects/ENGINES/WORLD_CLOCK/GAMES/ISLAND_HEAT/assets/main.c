#include <kos.h>
#include "frog_math.h"
#include "frog_physics.h"

// 1994 Camry 5S-FE Specs (Approximated for Dreamcast)
#define MAX_RPM 6200.0f
#define IDLE_RPM 8000.0f 
#define GEAR_REDUCTION 3.94f // Final drive ratio

typedef struct {
    float rpm;
    int gear;
    float clutch_pos;      // 0.0 (Released/Engaged) to 1.0 (Pressed/Disengaged)
    float throttle;        // 0.0 to 1.0
    float velocity;        // Actual movement speed
    float engine_load;
} camry_t;

camry_t my_car = { 800.0f, 1, 0.0f, 0.0f, 0.0f, 0.0f };

void apply_camry_physics() {
    // 1. Calculate Engine Torque based on RPM
    // The 5S-FE peaks around 4400 RPM
    float torque = (my_car.rpm / MAX_RPM) * 145.0f; 

    // 2. Clutch Engagement Logic
    // If clutch_pos is 0, transmission is locked to engine.
    // If clutch_pos is 1, engine spins free.
    float engagement = 1.0f - my_car.clutch_pos;

    // 3. The "Slip" Calculation
    // This determines how much torque actually moves the car vs just revving the engine
    float drive_force = torque * engagement * (1.0f / (float)my_car.gear);
    
    // 4. Update Velocity (Adding Camry "Heaviness")
    float inertia = 1300.0f; // Weight in kg
    my_car.velocity += (drive_force / inertia);
    my_car.velocity *= 0.98f; // Natural rolling resistance/friction

    // 5. Update RPM based on Throttle vs Load
    if (my_car.clutch_pos > 0.5f) {
        // Disengaged: Engine revs freely and fast
        my_car.rpm += (my_car.throttle * 200.0f) - (my_car.rpm * 0.05f);
    } else {
        // Engaged: Engine RPM is tied to Wheel Speed
        my_car.rpm = (my_car.velocity * GEAR_REDUCTION * my_car.gear) + 800.0f;
    }

    // Safety: Don't let it stall or explode
    if (my_car.rpm < 800.0f) my_car.rpm = 800.0f;
    if (my_car.rpm > MAX_RPM) my_car.rpm = MAX_RPM;
}

void update_controls() {
    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if(cont) {
        cont_state_t *state = (cont_state_t *)maple_dev_status(cont);
        
        my_car.clutch_pos = state->ltrig / 255.0f; // L-Trig is Clutch
        my_car.throttle = state->rtrig / 255.0f;   // R-Trig is Gas

        // Simple Shifting
        if(state->buttons & CONT_Y) { if(my_car.gear < 5) my_car.gear++; }
        if(state->buttons & CONT_X) { if(my_car.gear > 1) my_car.gear--; }
    }
}

int main(int argc, char **argv) {
    // Initializing the World Clock
    // frog_graphics_init(); 

    while(1) {
        update_controls();
        apply_camry_physics();
        
        // Print Stats to Terminal for Debugging the "Feel"
        printf("RPM: %.0f | Gear: %d | Speed: %.2f | Clutch: %.2f\n", 
                my_car.rpm, my_car.gear, my_car.velocity, my_car.clutch_pos);
        
        timer_spin_sleep(16); // Aim for 60fps
    }
    return 0;
}
