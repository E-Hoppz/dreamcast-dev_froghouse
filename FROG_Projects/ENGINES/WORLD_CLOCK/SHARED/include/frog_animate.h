#ifndef FROG_ANIMATE_H
#define FROG_ANIMATE_H

#include <stdbool.h>
#include "frog_matrix.h"
#include "world_clock.h"
#include "frog_mesh.h"

// Standard Quaternion structure for rotations
typedef struct {
    float x, y, z, w;
} quat_t;

// [NEW] Dual Quaternion structure representing both rotation and translation
typedef struct {
    quat_t real; // Rotation quaternion
    quat_t dual; // Translation dual-quaternion
} dual_quat_t;

typedef struct { dual_quat_t transform; } FrogPose;

typedef struct {
    uint32_t duration_samples; 
    bool     looping;
} FrogAnimationClip;

typedef struct {
    uint32_t start_sample;     
    bool     is_playing;
    float    current_progress; 
} FrogAnimator;

// --- [ DUAL QUATERNION MATH OPERATIONS ] ---
dual_quat_t dq_from_parts(quat_t r, vec3_t t);
dual_quat_t dq_normalize(dual_quat_t dq);
dual_quat_t dq_blend(dual_quat_t dq1, dual_quat_t dq2, float t);
vec3_t      dq_transform_vector(dual_quat_t dq, vec3_t p);

// --- [ ANIMATION LOOPS ] ---
void frog_animate_start(FrogAnimator *anim, WorldClock *wc);
dual_quat_t frog_animate_blend(FrogPose *start, FrogPose *end, FrogAnimator *anim, FrogAnimationClip *clip, WorldClock *wc);

// [NEW] Dual Quaternion Skeletal Skinning pipeline
void frog_animate_skin_mesh(FrogMesh *mesh, dual_quat_t *bone_transforms, uint32_t bone_count);

#endif
