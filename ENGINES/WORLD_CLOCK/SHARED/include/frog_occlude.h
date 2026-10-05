#ifndef FROG_OCCLUDE_H
#define FROG_OCCLUDE_H

#include <stdint.h>
#include <stdbool.h>
#include "frog_math_3d.h"
#include <dc/matrix.h>

#define OCCLUSION_GRID_W 20 // 640 / 32
#define OCCLUSION_GRID_H 15 // 480 / 32

typedef struct {
    float depth_buffer[OCCLUSION_GRID_H][OCCLUSION_GRID_W];
} FrogTileBuffer;

typedef struct {
    vec3_t  *vertices;
    uint32_t vertex_count;
    float    min_z;
} FrogOccluderProxy;

void frog_occlude_clear_buffer(FrogTileBuffer *buffer);
void frog_occlude_rasterize_proxy(FrogTileBuffer *buffer, FrogOccluderProxy *proxy, matrix_t *world_view_proj);
bool frog_occlude_is_visible(FrogTileBuffer *buffer, vec3_t bounding_center, float radius, matrix_t *world_view_proj);

#endif
