#ifndef VEHICLE_H
#define VEHICLE_H

#include <kos.h>

// THE MASTER SPEC (from your Doc)
typedef struct {
    float rpm;              // Engine revolutions
    float velocity;         // Ground speed
    int gear;               // Current gear (0-6)
    float clutch_pos;       // 0.0 (Released) to 1.0 (Fully Pulled)
    float throttle_pos;     // 0.0 to 1.0 (Right Trigger)
    float brake_pressure;   // 0.0 to 1.0 (A-Button Ramp)
    int is_stalled;         // Boolean flag
} VehicleState;

// This tells the compiler "Someone, somewhere has the actual car."
extern VehicleState player_car;

#endif

