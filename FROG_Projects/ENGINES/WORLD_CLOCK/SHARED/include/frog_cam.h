#ifndef FROG_CAM_H
#define FROG_CAM_H

#include "frog_math_3d.h"
#include "world_clock.h"

#define FROG_CAM_2D 0
#define FROG_CAM_3D 1

typedef struct {
    int    mode;            
    vec3_t position;        
    vec3_t target;          
    vec3_t up;              
    
    float fov;              
    float distance;         
    float lerp_speed;       
    float shake_intensity;  
    
    // --- [ THE SPLIT SCREEN VIEWPORTS ] ---
    float vp_x; 
    float vp_y; 
    float vp_w; 
    float vp_h; 
} frog_cam_t;

void frog_cam_init(frog_cam_t *cam, int mode);
void frog_cam_set_viewport(frog_cam_t *cam, float x, float y, float w, float h);
void frog_cam_update(frog_cam_t *cam, vec3_t target_pos, WorldClock *wc);
void frog_cam_apply(frog_cam_t *cam); 

#endif
