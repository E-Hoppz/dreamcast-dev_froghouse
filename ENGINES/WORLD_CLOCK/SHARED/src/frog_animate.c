//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_animate.h"
#include "frog_math.h"
#include "frog_matrix.h"
#include <dc/fmath.h> // Access the fast-math frsqrt assembly intrinsic
#include <string.h>
#include <math.h>

// --- [ DUAL QUATERNION MATH OPERATIONS ] ---

dual_quat_t dq_from_parts(quat_t r, vec3_t t) {
    dual_quat_t dq;
    dq.real = r;
    // q_d = 0.5 * t * q_r
    dq.dual.w = -0.5f * (t.x * r.x + t.y * r.y + t.z * r.z);
    dq.dual.x =  0.5f * (t.x * r.w + t.y * r.z - t.z * r.y);
    dq.dual.y =  0.5f * (t.y * r.w + t.z * r.x - t.x * r.z);
    dq.dual.z =  0.5f * (t.z * r.w + t.x * r.y - t.y * r.x);
    return dq;
}

dual_quat_t dq_normalize(dual_quat_t dq) {
    float mag_sq = dq.real.x * dq.real.x + dq.real.y * dq.real.y + dq.real.z * dq.real.z + dq.real.w * dq.real.w;
    if (mag_sq < 0.0001f) return dq;
    
    // Upgraded to single-cycle SH-4 hardware reciprocal square root
    float inv_mag = frsqrt(mag_sq);
    dq.real.x *= inv_mag;
    dq.real.y *= inv_mag;
    dq.real.z *= inv_mag;
    dq.real.w *= inv_mag;

    float dot_rd = dq.real.x * dq.dual.x + dq.real.y * dq.dual.y + dq.real.z * dq.dual.z + dq.real.w * dq.dual.w;
    float scale = dot_rd * inv_mag * inv_mag;

    dq.dual.x = (dq.dual.x * inv_mag) - (dq.real.x * scale);
    dq.dual.y = (dq.dual.y * inv_mag) - (dq.real.y * scale);
    dq.dual.z = (dq.dual.z * inv_mag) - (dq.real.z * scale);
    dq.dual.w = (dq.dual.w * inv_mag) - (dq.real.w * scale);

    return dq;
}

dual_quat_t dq_blend(dual_quat_t dq1, dual_quat_t dq2, float t) {
    float dot = dq1.real.x * dq2.real.x + 
                dq1.real.y * dq2.real.y + 
                dq1.real.z * dq2.real.z + 
                dq1.real.w * dq2.real.w;

    float w1 = 1.0f - t;
    float w2 = t;

    if (dot < 0.0f) {
        w2 = -t;
    }

    dual_quat_t blended;
    blended.real.x = dq1.real.x * w1 + dq2.real.x * w2;
    blended.real.y = dq1.real.y * w1 + dq2.real.y * w2;
    blended.real.z = dq1.real.z * w1 + dq2.real.z * w2;
    blended.real.w = dq1.real.w * w1 + dq2.real.w * w2;

    blended.dual.x = dq1.dual.x * w1 + dq2.dual.x * w2;
    blended.dual.y = dq1.dual.y * w1 + dq2.dual.y * w2;
    blended.dual.z = dq1.dual.z * w1 + dq2.dual.z * w2;
    blended.dual.w = dq1.dual.w * w1 + dq2.dual.w * w2;

    return dq_normalize(blended);
}

vec3_t dq_transform_vector(dual_quat_t dq, vec3_t p) {
    vec3_t r = {dq.real.x, dq.real.y, dq.real.z, 0.0f};
    float  w = dq.real.w;
    vec3_t d = {dq.dual.x, dq.dual.y, dq.dual.z, 0.0f};
    float  wd = dq.dual.w;

    vec3_t r_xp = vec3_cross(r, p);
    vec3_t t1 = vec3_add(r_xp, vec3_scale(p, w));
    vec3_t t2 = vec3_cross(r, t1);

    vec3_t r_xd = vec3_cross(r, d);
    vec3_t t3 = vec3_add(vec3_scale(r, wd), vec3_sub(r_xd, vec3_scale(d, w)));

    vec3_t out = vec3_add(p, vec3_scale(vec3_add(t2, t3), 2.0f));
    out.w = 1.0f;
    return out;
}

// --- [ ANIMATION LOOPS ] ---

void frog_animate_start(FrogAnimator *anim, WorldClock *wc) {
    if (!anim || !wc) return;
    anim->start_sample = wc->audio.sample_count;
    anim->is_playing = true;
    anim->current_progress = 0.0f;
}

dual_quat_t frog_animate_blend(FrogPose *start, FrogPose *end, FrogAnimator *anim, FrogAnimationClip *clip, WorldClock *wc) {
    if (!anim->is_playing) return start->transform;

    uint32_t elapsed = wc->audio.sample_count - anim->start_sample;
    float t = (float)elapsed / (float)clip->duration_samples;

    if (t >= 1.0f) {
        if (clip->looping) {
            anim->start_sample = wc->audio.sample_count;
            t = 0.0f;
        } else {
            anim->is_playing = false;
            anim->current_progress = 1.0f;
            return end->transform;
        }
    }

    anim->current_progress = t;
    float colani_t = frog_math_colani_lerp(0.0f, 1.0f, t);

    return dq_blend(start->transform, end->transform, colani_t);
}

// Optimized skeletal skinning utilizing cache-prefetching and division-free loops
void frog_animate_skin_mesh(FrogMesh *mesh, dual_quat_t *bone_transforms, uint32_t bone_count) {
    if (!mesh || !bone_transforms || bone_count == 0) return;

    register uint32_t bone_idx = 0;

    for (uint32_t i = 0; i < mesh->vertex_count; i++) {
        // Prefetch vertex memory two steps ahead to prevent bus stalling
        if (i < mesh->vertex_count - 2) {
            __asm__ __volatile__("pref @%0" : : "r"(&mesh->vertices[i + 2]));
        }

        FrogVertex *v = &mesh->vertices[i];
        vec3_t original_pos = { v->x, v->y, v->z, 1.0f };
        
        // Division-free index tracker (Replacing expensive i % bone_count)
        dual_quat_t dq = bone_transforms[bone_idx];
        bone_idx++;
        if (bone_idx >= bone_count) {
            bone_idx = 0;
        }

        vec3_t skinned_pos = dq_transform_vector(dq, original_pos);

        v->x = skinned_pos.x;
        v->y = skinned_pos.y;
        v->z = skinned_pos.z;
    }
}
