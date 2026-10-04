#ifndef FROG_COLLISION_3D_H
#define FROG_COLLISION_3D_H

#include "frog_math_3d.h"
#include "frog_physics.h" 
#include <stdbool.h>

#define GRID_CELL_SIZE 32.0f
#define GRID_SHIFT_BITS 21 // 16 (for 16.16 fractional) + 5 (since 32 = 2^5)
#define GRID_HASH_SIZE 128
#define MAX_ENTITIES_PER_CELL 16

typedef int32_t fixed16_t;

typedef struct {
    fixed16_t x, y, z;
} vec3_fixed_t;

typedef struct {
    uint32_t entity_ids[MAX_ENTITIES_PER_CELL];
    uint32_t count;
} FrogGridCell;

typedef struct {
    FrogGridCell cells[GRID_HASH_SIZE];
} FrogSpatialGrid;

// [1] Sphere
typedef struct {
    vec3_t center;
    float  radius;
} frog_sphere_t;

// [2] AABB
typedef struct {
    vec3_t min; 
    vec3_t max; 
} frog_aabb_t;

// [3] [NEW] Oriented Bounding Box (OBB - For rotating car collision)
typedef struct {
    vec3_t center;
    vec3_t axes[3];       // Local coordinate system (orthogonal directions)
    vec3_t half_extents;  // Extents along each axis
} frog_obb_t;

// [4] [NEW] 3D Signed Distance Field (SDF) Voxel Grid
typedef struct {
    float  distance; // Distance to nearest surface (negative = inside)
    vec3_t normal;   // Surface normal vector pointing away from geometry
} FrogSDFVoxel;

typedef struct {
    FrogSDFVoxel *voxels;
    vec3_t        min_bounds;
    vec3_t        max_bounds;
    uint32_t      dim_x, dim_y, dim_z;
    float         voxel_size;
} FrogSDFGrid;

// --- [ COLLISION GRID SHUNTS ] ---
void frog_collision_grid_clear(FrogSpatialGrid *grid);
void frog_collision_grid_insert(FrogSpatialGrid *grid, vec3_t pos, uint32_t entity_id);
void frog_collision_grid_query(FrogSpatialGrid *grid, vec3_t pos, uint32_t *out_ids, uint32_t *out_count);

// --- [ NEW SDF & OBB PHYSICS SHUNTS ] ---
void frog_collision_sdf_init(FrogSDFGrid *grid, vec3_t min_b, vec3_t max_b, float v_size);
bool frog_collision_sdf_check(FrogSDFGrid *grid, vec3_t pos, float radius, float *out_penetration, vec3_t *out_normal);
bool frog_check_obb_v_obb(frog_obb_t *a, frog_obb_t *b);

// --- [ COLLISION DETECTORS ] ---
bool frog_check_sphere_v_sphere(frog_sphere_t a, frog_sphere_t b, FrogBody *b1, FrogBody *b2);
bool frog_check_aabb_v_aabb(frog_aabb_t a, frog_aabb_t b, FrogBody *b1);
bool frog_check_sphere_v_aabb(frog_sphere_t s, frog_aabb_t b, FrogBody *b1);

void frog_collision_set_params(float sensitivity, float vibe);

#endif
