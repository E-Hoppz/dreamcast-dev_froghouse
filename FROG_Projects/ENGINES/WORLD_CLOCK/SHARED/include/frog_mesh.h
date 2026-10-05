#ifndef FROG_MESH_H
#define FROG_MESH_H

#include <stdint.h>
#include <dc/pvr.h>
#include "frog_asset.h"

/**
 * [REPAIRMAN'S SPEC]
 * A 32-byte hardware-aligned vertex.
 * Explicit padding is added to guarantee structural alignment 
 * across both macOS Clang and KOS GCC compilers.
 */
typedef struct {
    float x, y, z;      // 12 bytes - Position Signal
    float u, v;         // 8 bytes  - Texture Signal
    uint32_t argb;      // 4 bytes  - Color Signal
    uint32_t oargb;     // 4 bytes  - Nemo Glow Signal
    uint32_t padding;   // 4 bytes - Explicit 32-byte alignment pad
} __attribute__((aligned(32))) FrogVertex;

// [REPAIRMAN'S SPEC] The official binary header preceding your .fmg model files.
typedef struct {
    uint32_t magic;         // Magic bytes (e.g. 0x464d4731 - "FMG1")
    uint32_t vertex_count;  // Total vertices following this header
    uint32_t flags;         // Bit 0 = Transparent, Bit 1 = Punch-Through
    float    radius;        // Bounding sphere radius for culling
    uint32_t mipmap_bias;   // Mip-Map Bias Skewing offset (0 to 15, default PVR_MIPBIAS_NORMAL)
} FrogMeshHeader;

typedef struct FrogMesh {
    FrogVertex *vertices;
    uint32_t    vertex_count;
    struct FrogMesh *lod_fallback; // Link to low-poly variant for distant rendering
    frog_texture_t *texture;
    bool        is_transparent;
    bool        is_punch_thru; // 1-bit alpha flag (shunted to PT list, bypassing Radix sort)
    uint32_t    mipmap_bias;   // Hardware mipmap bias register value
    float       radius; 
    int         contract_override; // [NEW] -1 = Use Global Engine default, 0 = Opaque, 1 = Glide, 2 = MIPS, etc.
} FrogMesh;

FrogMesh* frog_mesh_load(const char *path, frog_texture_t *tex);
void frog_mesh_render(FrogMesh *mesh, bool filter);
void frog_mesh_unload(FrogMesh *mesh);

#endif
