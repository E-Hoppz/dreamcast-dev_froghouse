#include "frog_occlude.h"
#include <string.h>
#include <math.h>

void frog_occlude_clear_buffer(FrogTileBuffer *buffer) {
    if (buffer) {
        // Clear software depth tile buffer to infinite distance
        for (int y = 0; y < OCCLUSION_GRID_H; y++) {
            for (int x = 0; x < OCCLUSION_GRID_W; x++) {
                buffer->depth_buffer[y][x] = 1000.0f;
            }
        }
    }
}

void frog_occlude_rasterize_proxy(FrogTileBuffer *buffer, FrogOccluderProxy *proxy, matrix_t *world_view_proj) {
    if (!buffer || !proxy || !world_view_proj) return;

    mat_load(world_view_proj);

    // Find the projected bounding area on our 20x15 grid
    float min_x = 10000.0f, max_x = -10000.0f;
    float min_y = 10000.0f, max_y = -10000.0f;
    float min_z = 10000.0f;

    for (uint32_t i = 0; i < proxy->vertex_count; i++) {
        float x = proxy->vertices[i].x;
        float y = proxy->vertices[i].y;
        float z = proxy->vertices[i].z;

        mat_trans_single(x, y, z); // Project to screen coordinates

        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        if (z < min_z) min_z = z;
    }

    // Convert screen coordinates to grid coordinates (Dividing by 32)
    int start_x = (int)(min_x / 32.0f);
    int end_x   = (int)(max_x / 32.0f);
    int start_y = (int)(min_y / 32.0f);
    int end_y   = (int)(max_y / 32.0f);

    // Clamp grid boundaries
    if (start_x < 0) start_x = 0;
    if (end_x >= OCCLUSION_GRID_W) end_x = OCCLUSION_GRID_W - 1;
    if (start_y < 0) start_y = 0;
    if (end_y >= OCCLUSION_GRID_H) end_y = OCCLUSION_GRID_H - 1;

    // Conservative rasterization: Write the proxy's near Z depth into covered tiles
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            if (min_z < buffer->depth_buffer[y][x]) {
                buffer->depth_buffer[y][x] = min_z;
            }
        }
    }
}

bool frog_occlude_is_visible(FrogTileBuffer *buffer, vec3_t bounding_center, float radius, matrix_t *world_view_proj) {
    if (!buffer || !world_view_proj) return true;

    // Project bounding sphere center
    float sx = bounding_center.x;
    float sy = bounding_center.y;
    float sz = bounding_center.z;

    mat_load(world_view_proj);
    mat_trans_single(sx, sy, sz);

    // If the object is behind the camera near-plane, cull it immediately
    if (sz < 0.1f) return false;

    // Calculate approximate screen-space boundary of the sphere
    int start_x = (int)((sx - radius) / 32.0f);
    int end_x   = (int)((sx + radius) / 32.0f);
    int start_y = (int)((sy - radius) / 32.0f);
    int end_y   = (int)((sy + radius) / 32.0f);

    if (start_x < 0) start_x = 0;
    if (end_x >= OCCLUSION_GRID_W) end_x = OCCLUSION_GRID_W - 1;
    if (start_y < 0) start_y = 0;
    if (end_y >= OCCLUSION_GRID_H) end_y = OCCLUSION_GRID_H - 1;

    // Check if the bounding sphere is completely behind occluding geometry in all covered tiles
    bool fully_occluded = true;
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x <= end_x; x++) {
            if (sz <= buffer->depth_buffer[y][x]) {
                fully_occluded = false; // Closer than occluder, visible!
                break;
            }
        }
        if (!fully_occluded) break;
    }

    return !fully_occluded;
}
