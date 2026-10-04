#ifndef FROG_BATCH_H
#define FROG_BATCH_H

#include <kos.h>
#include <dc/pvr.h>

#define MAX_BATCH_PARTICLES 1024

typedef struct {
    pvr_vertex_t vertices[MAX_BATCH_PARTICLES * 4]; // 4 vertices per quad
    uint32_t count;
    pvr_poly_hdr_t header;
} frog_batch_t;

// Initialize the batch with a specific texture/color header
void frog_batch_init(frog_batch_t *batch, pvr_poly_hdr_t *hdr);

// Add a "Trance" particle to the bucket
void frog_batch_add(frog_batch_t *batch, float x, float y, float size, uint32_t color);

// Ship the entire bucket to the PVR hardware
void frog_batch_flush(frog_batch_t *batch);

#endif
