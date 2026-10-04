//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_input.h"
#include "world_clock.h" 
#include "frog_loader.h" 
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/mouse.h>
#include <dc/maple/keyboard.h>
#include <dc/vblank.h> 
#include <arch/irq.h> 
#include <math.h>

extern bool frog_loader_is_g2_locked(void);

static FrogInput cached_inputs;
static volatile bool input_cache_ready = false;

// Triggered during V-Blank with single-cycle reciprocal float multiplications
void frog_input_cache_dma(void) {
    if (frog_loader_is_g2_locked()) {
        return; 
    }

    FrogInput temp_cache;
    __builtin_memset(&temp_cache, 0, sizeof(FrogInput));

    maple_device_t *cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if (cont) {
        cont_state_t *raw = (cont_state_t *)maple_dev_status(cont);
        if (raw) {
            temp_cache.btn_a      = (raw->buttons & CONT_A);
            temp_cache.btn_b      = (raw->buttons & CONT_B);
            temp_cache.btn_x      = (raw->buttons & CONT_X);
            temp_cache.btn_y      = (raw->buttons & CONT_Y);
            temp_cache.start      = (raw->buttons & CONT_START);
            temp_cache.dpad_up    = (raw->buttons & CONT_DPAD_UP);
            temp_cache.dpad_down  = (raw->buttons & CONT_DPAD_DOWN);
            temp_cache.dpad_left  = (raw->buttons & CONT_DPAD_LEFT);
            temp_cache.dpad_right = (raw->buttons & CONT_DPAD_RIGHT);

            // Replaced divisions with single-cycle reciprocal multiplications
            temp_cache.axis_l = (float)raw->ltrig * 0.00392156862f; // * 1/255.0
            temp_cache.axis_r = (float)raw->rtrig * 0.00392156862f;

            float sx = (float)raw->joyx * 0.0078125f; // * 1/128.0
            float sy = (float)raw->joyy * 0.0078125f;
            temp_cache.stick_x = (fabsf(sx) > 0.1f) ? sx : 0.0f;
            temp_cache.stick_y = (fabsf(sy) > 0.1f) ? sy : 0.0f;

            temp_cache.action_primary = temp_cache.btn_a || (temp_cache.axis_r > 0.5f);
        }
    }

    maple_device_t *mouse_dev = maple_enum_type(0, MAPLE_FUNC_MOUSE);
    if (mouse_dev) {
        mouse_state_t *m_raw = (mouse_state_t *)maple_dev_status(mouse_dev);
        if (m_raw) {
            temp_cache.is_mouse = 1;
            temp_cache.mouse_dx = (float)m_raw->dx;
            temp_cache.mouse_dy = (float)m_raw->dy;
        }
    }

    maple_device_t *kb_dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
    if (kb_dev) {
        temp_cache.is_kb = 1;
    }

    int old_irq = irq_disable();
    cached_inputs = temp_cache;
    input_cache_ready = true;
    irq_restore(old_irq);
}

static void vblank_callback(uint32_t code, void *data) {
    (void)code; (void)data;
    frog_input_cache_dma();
}

void frog_input_init(void) {
    vblank_handler_add(vblank_callback, NULL);
}

void frog_input_update(FrogInput *frog, struct WorldClock *wc) {
    if (!frog || !wc) return;

    frog->timestamp = wc->audio.sample_count;

    if (input_cache_ready) {
        int old_irq = irq_disable(); 
        
        frog->btn_a      = cached_inputs.btn_a;
        frog->btn_b      = cached_inputs.btn_b;
        frog->btn_x      = cached_inputs.btn_x;
        frog->btn_y      = cached_inputs.btn_y;
        frog->start      = cached_inputs.start;
        frog->dpad_up    = cached_inputs.dpad_up;
        frog->dpad_down  = cached_inputs.dpad_down;
        frog->dpad_left  = cached_inputs.dpad_left;
        frog->dpad_right = cached_inputs.dpad_right;

        frog->axis_l         = cached_inputs.axis_l;
        frog->axis_r         = cached_inputs.axis_r;
        frog->stick_x        = cached_inputs.stick_x;
        frog->stick_y        = cached_inputs.stick_y;
        frog->action_primary = cached_inputs.action_primary;
        
        frog->is_mouse = cached_inputs.is_mouse;
        frog->mouse_dx = cached_inputs.mouse_dx;
        frog->mouse_dy = cached_inputs.mouse_dy;
        frog->is_kb    = cached_inputs.is_kb;
        
        irq_restore(old_irq);
    }
}
