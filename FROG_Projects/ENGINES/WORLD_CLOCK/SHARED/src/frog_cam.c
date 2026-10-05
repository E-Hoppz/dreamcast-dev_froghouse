#include <stdlib.h>
#include <math.h>
#include <dc/matrix.h>
#include <dc/matrix3d.h> 
#include "frog_cam.h"
#include "frog_matrix.h"
#include "frog_math.h"
#include "world_clock.h"
#include "frog_physics.h"

static mat4_t camera_matrix;

void frog_cam_init(frog_cam_t *cam, int mode) {
    cam->mode = mode; 
    cam->position = (vec3_t){0, 0, 5, 1.0f};
    cam->target = (vec3_t){0, 0, 0, 1.0f};
    cam->up = (vec3_t){0, 1, 0, 0.0f};
    cam->shake_intensity = 0.0f;
    cam->lerp_speed = 0.1f;
    cam->distance = 10.0f;
    
    // Default to full screen
    cam->vp_x = 0.0f;
    cam->vp_y = 0.0f;
    cam->vp_w = 1.0f;
    cam->vp_h = 1.0f;
}

void frog_cam_set_viewport(frog_cam_t *cam, float x, float y, float w, float h) {
    if (!cam) return;
    cam->vp_x = x;
    cam->vp_y = y;
    cam->vp_w = w;
    cam->vp_h = h;
}

void frog_cam_update(frog_cam_t *cam, vec3_t target_pos, WorldClock *wc) {
    if(!cam || !wc) return;

    if (cam->shake_intensity > 0.05f) {
        float vibe = frog_physics_get_resonance_jitter(wc);
        cam->position.x += vibe * cam->shake_intensity;
        cam->position.y += vibe * cam->shake_intensity;
        cam->shake_intensity *= 0.9f; 
    }

    cam->target.x = frog_math_colani_lerp(cam->target.x, target_pos.x, cam->lerp_speed);
    cam->target.y = frog_math_colani_lerp(cam->target.y, target_pos.y, cam->lerp_speed);
    cam->target.z = frog_math_colani_lerp(cam->target.z, target_pos.z, cam->lerp_speed);

    if (cam->mode == FROG_CAM_3D) {
        cam->position.x = cam->target.x;
        cam->position.y = cam->target.y + (cam->distance * 0.5f);
        cam->position.z = cam->target.z - cam->distance;

        // [REPAIRMAN'S SHUNT] Bounding check: Prevent camera from clipping below the water
        float water_y = frog_physics_get_wave_height(cam->position.x, cam->position.z, wc);
        if (cam->position.y < (water_y + 10.0f)) {
            cam->position.y = water_y + 10.0f; 
        }

        mat_identity();
        mat_lookat((point_t *)&cam->position, 
                   (point_t *)&cam->target, 
                   (vector_t *)&cam->up);
                   
        mat_store((matrix_t *)camera_matrix.m);
    } else {
        camera_matrix = mat4_translate(-cam->target.x, -cam->target.y, 0.0f);
    }
}

void frog_cam_apply(frog_cam_t *cam) {
    (void)cam;
    mat_load((matrix_t *)camera_matrix.m);
}
