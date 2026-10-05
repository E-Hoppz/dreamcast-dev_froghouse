#include "frog_batch.h"
#include <string.h>

void frog_batch_init(frog_batch_t *batch, pvr_poly_hdr_t *hdr) {
    batch->count = 0;
    memcpy(&batch->header, hdr, sizeof(pvr_poly_hdr_t));
}

void frog_batch_add(frog_batch_t *batch, float x, float y, float size, uint32_t color) {
    if (batch->count >= MAX_BATCH_PARTICLES) return;

    int idx = batch->count * 4;
    float h_size = size / 2.0f;

    // Populate the 4 corners of the quad in the SH-4's fast buffer
    for(int i = 0; i < 4; i++) {
        batch->vertices[idx + i].argb = color;
        batch->vertices[idx + i].z = 1.0f;
        batch->vertices[idx + i].flags = PVR_CMD_VERTEX;
    }

    // Triangle Strip Layout
    batch->vertices[idx + 0].x = x - h_size; batch->vertices[idx + 0].y = y + h_size;
    batch->vertices[idx + 1].x = x - h_size; batch->vertices[idx + 1].y = y - h_size;
    batch->vertices[idx + 2].x = x + h_size; batch->vertices[idx + 2].y = y + h_size;
    batch->vertices[idx + 3].x = x + h_size; batch->vertices[idx + 3].y = y - h_size;

    batch->vertices[idx + 3].flags = PVR_CMD_VERTEX_EOL; // End of this specific quad

    batch->count++;
}

void frog_batch_flush(frog_batch_t *batch) {
    if (batch->count == 0) return;

    // Submit the header once
    pvr_prim(&batch->header, sizeof(batch->header));

    // Pump the entire vertex array to the PVR
    for (uint32_t i = 0; i < batch->count * 4; i++) {
        pvr_prim(&batch->vertices[i], sizeof(pvr_vertex_t));
    }

    batch->count = 0; // Empty the bucket for the next frame
}
