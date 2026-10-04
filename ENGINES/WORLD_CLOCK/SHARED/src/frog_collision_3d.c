//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_collision_3d.h"
#include "frog_audio.h"
#include "frog_physics.h"
#include "frog_util.h"   
#include <dc/fmath.h> // Fast-math reciprocal square root
#include <math.h>
#include <string.h>
#include <stdlib.h>

static float impact_sensitivity = 1.0f;
static float vibration_multiplier = 0.5f;

void frog_collision_handle_impact(float force, int material_type);

static uint32_t grid_hash(int32_t cx, int32_t cy, int32_t cz) {
    uint32_t h = (uint32_t)((cx * 73856093) ^ (cy * 19349663) ^ (cz * 83492791));
    return h % GRID_HASH_SIZE;
}

void frog_collision_grid_clear(FrogSpatialGrid *grid) {
    if (grid) {
        memset(grid->cells, 0, sizeof(grid->cells));
    }
}

void frog_collision_grid_insert(FrogSpatialGrid *grid, vec3_t pos, uint32_t entity_id) {
    if (!grid) return;

    // Convert coordinates using fast reciprocal operations to bypass float division
    int32_t cx = (int32_t)(pos.x * 65536.0f) >> GRID_SHIFT_BITS;
    int32_t cy = (int32_t)(pos.y * 65536.0f) >> GRID_SHIFT_BITS;
    int32_t cz = (int32_t)(pos.z * 65536.0f) >> GRID_SHIFT_BITS;

    uint32_t h = grid_hash(cx, cy, cz);
    FrogGridCell *cell = &grid->cells[h];

    if (cell->count < MAX_ENTITIES_PER_CELL) {
        cell->entity_ids[cell->count++] = entity_id;
    }
}

void frog_collision_grid_query(FrogSpatialGrid *grid, vec3_t pos, uint32_t *out_ids, uint32_t *out_count) {
    if (!grid || !out_ids || !out_count) return;

    int32_t cx = (int32_t)(pos.x * 65536.0f) >> GRID_SHIFT_BITS;
    int32_t cy = (int32_t)(pos.y * 65536.0f) >> GRID_SHIFT_BITS;
    int32_t cz = (int32_t)(pos.z * 65536.0f) >> GRID_SHIFT_BITS;

    uint32_t h = grid_hash(cx, cy, cz);
    FrogGridCell *cell = &grid->cells[h];

    *out_count = cell->count;
    memcpy(out_ids, cell->entity_ids, cell->count * sizeof(uint32_t));
}

void frog_collision_sdf_init(FrogSDFGrid *grid, vec3_t min_b, vec3_t max_b, float v_size) {
    if (!grid) return;
    grid->min_bounds = min_b;
    grid->max_bounds = max_b;
    grid->voxel_size = v_size;
    
    // Division-free reciprocal step calculated once during setup
    float inv_size = 1.0f / v_size;
    grid->dim_x = (uint32_t)((max_b.x - min_b.x) * inv_size);
    grid->dim_y = (uint32_t)((max_b.y - min_b.y) * inv_size);
    grid->dim_z = (uint32_t)((max_b.z - min_b.z) * inv_size);
    
    uint32_t total_voxels = grid->dim_x * grid->dim_y * grid->dim_z;
    grid->voxels = (FrogSDFVoxel *)malloc(total_voxels * sizeof(FrogSDFVoxel));
    memset(grid->voxels, 0, total_voxels * sizeof(FrogSDFVoxel));
}

bool frog_collision_sdf_check(FrogSDFGrid *grid, vec3_t pos, float radius, float *out_penetration, vec3_t *out_normal) {
    if (!grid || !grid->voxels || !out_penetration || !out_normal) return false;

    // Direct look-up using pre-calculated reciprocal step
    float inv_size = 1.0f / grid->voxel_size;
    int gx = (int)((pos.x - grid->min_bounds.x) * inv_size);
    int gy = (int)((pos.y - grid->min_bounds.y) * inv_size);
    int gz = (int)((pos.z - grid->min_bounds.z) * inv_size);

    if (gx < 0 || gx >= (int)grid->dim_x ||
        gy < 0 || gy >= (int)grid->dim_y ||
        gz < 0 || gz >= (int)grid->dim_z) {
        return false;
    }

    uint32_t idx = (gz * grid->dim_y * grid->dim_x) + (gy * grid->dim_x) + gx;
    FrogSDFVoxel *voxel = &grid->voxels[idx];

    if (voxel->distance < radius) {
        *out_penetration = radius - voxel->distance;
        *out_normal = voxel->normal;
        return true;
    }

    return false;
}

// Optimized Oriented Bounding Box separating axis checking. 
// Removed 'register' storage class from R_mat to prevent pointer decay errors.
bool frog_check_obb_v_obb(frog_obb_t *a, frog_obb_t *b) {
    if (!a || !b) return false;

    vec3_t R = vec3_sub(b->center, a->center);
    float T[3];
    T[0] = vec3_dot(R, a->axes[0]);
    T[1] = vec3_dot(R, a->axes[1]);
    T[2] = vec3_dot(R, a->axes[2]);

    float R_mat[3][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R_mat[i][j] = vec3_dot(a->axes[i], b->axes[j]);
        }
    }

    float rA, rB;

    for (int i = 0; i < 3; i++) {
        rA = (&a->half_extents.x)[i];
        rB = b->half_extents.x * fabsf(R_mat[i][0]) + 
             b->half_extents.y * fabsf(R_mat[i][1]) + 
             b->half_extents.z * fabsf(R_mat[i][2]);
        if (fabsf(T[i]) > (rA + rB)) return false;
    }

    for (int i = 0; i < 3; i++) {
        rA = a->half_extents.x * fabsf(R_mat[0][i]) + 
             a->half_extents.y * fabsf(R_mat[1][i]) + 
             a->half_extents.z * fabsf(R_mat[2][i]);
        rB = (&b->half_extents.x)[i];
        float proj_T = T[0] * R_mat[0][i] + T[1] * R_mat[1][i] + T[2] * R_mat[2][i];
        if (fabsf(proj_T) > (rA + rB)) return false;
    }

    return true; 
}

bool frog_check_sphere_v_sphere(frog_sphere_t a, frog_sphere_t b, FrogBody *b1, FrogBody *b2) {
    vec3_t diff = vec3_sub(a.center, b.center);
    float dist_sq = vec3_dot(diff, diff); 
    float radius_sum = a.radius + b.radius;

    if (dist_sq <= (radius_sum * radius_sum)) {
        float force = vec3_length(vec3_sub(b1->velocity, b2->velocity));
        frog_collision_handle_impact(force, 0);
        return true;
    }
    return false;
}

bool frog_check_aabb_v_aabb(frog_aabb_t a, frog_aabb_t b, FrogBody *b1) {
    bool hit = (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);

    if (hit) {
        float force = vec3_length(b1->velocity);
        frog_collision_handle_impact(force, 1);
        return true;
    }
    return false;
}

bool frog_check_sphere_v_aabb(frog_sphere_t s, frog_aabb_t b, FrogBody *b1) {
    float x = fmaxf(b.min.x, fminf(s.center.x, b.max.x));
    float y = fmaxf(b.min.y, fminf(s.center.y, b.max.y));
    float z = fmaxf(b.min.z, fminf(s.center.z, b.max.z));

    vec3_t closest_point = {x, y, z};
    vec3_t diff = vec3_sub(closest_point, s.center);
    float dist_sq = vec3_dot(diff, diff);
    
    if (dist_sq <= (s.radius * s.radius)) {
        float force = vec3_length(b1->velocity);
        frog_collision_handle_impact(force, 2); 
        return true;
    }
    return false;
}

void frog_collision_set_params(float sensitivity, float vibe) {
    impact_sensitivity = sensitivity;
    vibration_multiplier = vibe;
}

void frog_collision_handle_impact(float force, int material_type) {
    (void)material_type;
    if (force > 1.0f) {
        frog_audio_trigger_momentum(GHOST_CH_VIBRATION, (uint32_t)(force * 10.0f));
    }
}
