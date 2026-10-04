#include "frog_input_gun.h"
#include <dc/maple.h>
#include <dc/maple/controller.h> // [REPAIRMAN'S FIX] Include to read gun button states

void frog_input_gun_update(FrogLightgun *gun, WorldClock *wc) {
    if (!gun || !wc) return;

    // 1. [REPAIRMAN'S FIX] Read coordinates directly using KOS's native lightgun driver
    int rx = -1, ry = -1;
    maple_gun_read_pos(&rx, &ry);

    if (rx >= 0 && ry >= 0) {
        gun->screen_x = rx;
        gun->screen_y = ry;
        gun->offscreen = false;
    } else {
        gun->offscreen = true;
    }

    // 2. Poll the lightgun's buttons (which are registered under the controller driver)
    maple_device_t *cont_dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if (cont_dev) {
        cont_state_t *raw = (cont_state_t *)maple_dev_status(cont_dev);
        if (raw) {
            // [REPAIRMAN'S FIX] Lightgun trigger maps to Controller Button A (CONT_A)
            bool current_trigger = (raw->buttons & CONT_A);
            
            if (current_trigger && !gun->trigger_pulled) {
                gun->trigger_sample = wc->audio.sample_count;
            }
            gun->trigger_pulled = current_trigger;
        }
    }
}

void frog_input_gun_get_ray(FrogLightgun *gun, vec3_t *out_origin, vec3_t *out_dir) {
    if (!gun || gun->offscreen) return;

    // Normalizing coordinates for 3D projection
    float norm_x = ((float)gun->screen_x / 640.0f) * 2.0f - 1.0f;
    float norm_y = -(((float)gun->screen_y / 480.0f) * 2.0f - 1.0f);

    *out_origin = (vec3_t){0.0f, 0.0f, 0.0f, 1.0f};
    *out_dir = vec3_normalize((vec3_t){norm_x, norm_y, -1.0f, 0.0f});
}
