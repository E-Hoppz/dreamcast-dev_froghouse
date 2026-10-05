#ifndef FROG_SWEEP_H
#define FROG_SWEEP_H

#include <stdbool.h>
#include "frog_math_3d.h"
#include "frog_physics.h"
#include "frog_collision_3d.h"

typedef struct {
    bool   has_hit;      // True if an intersection occurred along the path
    float  fraction;     // Time of impact (0.0 = start, 1.0 = target destination)
    vec3_t hit_point;    // Intersection coordinate
    vec3_t hit_normal;   // Normal vector of the impacted surface plane
} FrogTrace;

/**
 * [SWEPT AABB VS POLYGON PLANE]
 * Projects a moving bounding box along its velocity vector against a static plane.
 * Calculates exact intersection parameters to prevent high-speed wall-tunneling.
 */
FrogTrace frog_sweep_aabb_vs_plane(frog_aabb_t box, vec3_t velocity, vec3_t plane_point, vec3_t plane_normal);

/**
 * [SLIDING COLLISION RESOLUTION]
 * Takes an active body's velocity vector, calculates the surface plane impact,
 * and slides the body parallel along the collision normal.
 */
void frog_sweep_resolve_slide(FrogBody *body, FrogTrace *trace);

#endif
