#include "frog_config.h"
#include <stdio.h>

// Define the global instance
FrogConfig frog_engine_config;

void frog_config_init(void) {
    printf("FROGHOUSE: Initializing Global Config...\n");

    frog_engine_config.debug_mode = true;  
    frog_engine_config.vsync_enabled = true;
    frog_engine_config.global_volume = 0.8f;
    frog_engine_config.frame_count = 0;
    frog_engine_config.active_contract = CONTRACT_OPAQUE; // Default to Opaque

    printf("FROGHOUSE: Config Loaded (Target %d FPS)\n", TARGET_FPS);
}
