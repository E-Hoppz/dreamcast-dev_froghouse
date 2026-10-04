#include "frog_sweep.h"
#include <math.h>

FrogTrace frog_sweep_aabb_vs_plane(frog_aabb_t box, vec3_t velocity, vec3_t plane_point, vec3_t plane_normal) {
    FrogTrace trace;
    trace.has_hit = false;
    trace.fraction = 1.0f;
    trace.hit_point = (vec3_t){0, 0, 0, 0};
    trace.hit_normal = (vec3_t){0, 0, 0, 0};

    // Calculate extents along plane normal vector
    float half_ext_x = (box.max.x - box.min.x) * 0.5f;
    float half_ext_y = (box.max.y - box.min.y) * 0.5f;
    float half_ext_z = (box.max.z - box.min.z) * 0.5f;

    vec3_t center = {
        box.min.x + half_ext_x,
        box.min.y + half_ext_y,
        box.min.z + half_ext_z,
        0.0f
    };

    // Project half-extents onto plane normal
    float r = half_ext_x * fabsf(plane_normal.x) +
              half_ext_y * fabsf(plane_normal.y) +
              half_ext_z * fabsf(plane_normal.z);

    // Compute distance from box center to the plane
    float dot_center = vec3_dot(center, plane_normal);
    float dot_plane = vec3_dot(plane_point, plane_normal);
    float distance = dot_center - dot_plane;

    // Check projection of the velocity vector
    float denom = vec3_dot(velocity, plane_normal);

    if (fabsf(denom) < 0.0001f) {
        // Velocity is parallel to plane
        return trace;
    }

    float t0 = (distance > 0.0f) ? (distance - r) / -denom : (distance + r) / -denom;

    // Check if impact timeline falls within the current frame interval
    if (t0 >= 0.0f && t0 <= 1.0f) {
        trace.has_hit = true;
        trace.fraction = t0;
        trace.hit_normal = plane_normal;
        
        // hit_point = center + velocity * t0
        trace.hit_point = vec3_add(center, vec3_scale(velocity, t0));
    }

    return trace;
}

void frog_sweep_resolve_slide(FrogBody *body, FrogTrace *trace) {
    if (!body || !trace || !trace->has_hit) return;

    // Relocate position to the exact timeline coordinate of impact
    body->position = vec3_add(body->position, vec3_scale(body->velocity, trace->fraction));

    // Calculate sliding direction: slide_vel = velocity - normal * dot(velocity, normal)
    float dot_vel_norm = vec3_dot(body->velocity, trace->hit_normal);
    vec3_t slide_vector = vec3_sub(body->velocity, vec3_scale(trace->hit_normal, dot_vel_norm));

    // Apply friction/drag losses to slide velocity vector
    body->velocity = vec3_scale(slide_vector, 0.9f);
}
