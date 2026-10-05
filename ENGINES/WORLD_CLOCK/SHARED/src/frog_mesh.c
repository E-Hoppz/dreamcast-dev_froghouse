//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include "frog_mesh.h"
#include "frog_draw.h"
#include "frog_color.h"
#include "frog_util.h"
#include "frog_config.h" 
#include "world_clock.h" 
#include "frog_scratchpad.h"
#include <dc/matrix.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern WorldClock wc; // Link with active SPU motherboard

// Structure representing a single triangle face for the CPU Radix Sorter
typedef struct {
    FrogVertex v[3];
    float      depth;
} FrogFace;

// Pre-allocated static face-sorting arrays to completely bypass dynamic malloc/free loops
#define MAX_SORT_FACES 512
static FrogFace sort_buffer_primary[MAX_SORT_FACES];
static FrogFace sort_buffer_secondary[MAX_SORT_FACES];

// Linear-time 8-bit Radix Sorter to resolve translucent face depth blending (OIT)
static void radix_sort_faces(FrogFace *source, FrogFace *temp, uint32_t count) {
    uint32_t histogram[256];
    uint32_t offset[256];
    int pass;
    
    // Sort 32-bit float depths in 4 passes of 8 bits each
    for (pass = 0; pass < 4; pass++) {
        uint32_t shift = pass * 8;
        memset(histogram, 0, sizeof(histogram));
        uint32_t i;
        
        // Build histogram
        for (i = 0; i < count; i++) {
            union { float f; uint32_t u; } key;
            key.f = source[i].depth;
            uint32_t bucket = (key.u >> shift) & 0xFF;
            histogram[bucket]++;
        }
        
        // Calculate offsets
        offset[0] = 0;
        for (i = 1; i < 256; i++) {
            offset[i] = offset[i - 1] + histogram[i - 1];
        }
        
        // Distribute elements to temporary buffer
        for (i = 0; i < count; i++) {
            union { float f; uint32_t u; } key;
            key.f = source[i].depth;
            uint32_t bucket = (key.u >> shift) & 0xFF;
            uint32_t dest_idx = offset[bucket]++;
            temp[dest_idx] = source[i];
        }
        
        // Swap pointers
        FrogFace *swap = source;
        source = temp;
        temp = swap;
    }
}

FrogMesh* frog_mesh_load(const char *path, frog_texture_t *tex) {
    file_t f = fs_open(path, O_RDONLY);
    if(f == FILEHND_INVALID) {
        frog_util_log("MESH ERROR: Blueprint not found.");
        return NULL;
    }

    // Parse the formal binary header from the .fmg model files.
    FrogMeshHeader header;
    if (fs_read(f, &header, sizeof(FrogMeshHeader)) != sizeof(FrogMeshHeader)) {
        frog_util_log("MESH ERROR: Failed to read FMG header.");
        fs_close(f);
        return NULL;
    }

    FrogMesh *mesh = (FrogMesh*)malloc(sizeof(FrogMesh));
    mesh->vertex_count = header.vertex_count;
    mesh->vertices = (FrogVertex*)malloc(header.vertex_count * sizeof(FrogVertex));
    mesh->texture = tex;
    mesh->radius = header.radius;
    mesh->lod_fallback = NULL;
    mesh->contract_override = -1; // Default to global engine default

    // Decode flags bitmask (Bit 0 = Transparent, Bit 1 = Punch-Through)
    mesh->is_transparent = (header.flags & 0x01) != 0;
    mesh->is_punch_thru  = (header.flags & 0x02) != 0;
    
    // Load the baked Mip-Map Bias offset directly from the FMG file
    mesh->mipmap_bias = header.mipmap_bias;

    // Load actual vertices following the header
    fs_read(f, mesh->vertices, header.vertex_count * sizeof(FrogVertex));
    fs_close(f);

    return mesh;
}

void frog_mesh_render(FrogMesh *mesh, bool filter) {
    if(!mesh || !mesh->texture) return;

    // --- [ STEP 1: THE FRUSTUM BYPASS ] ---
    float cx = 0, cy = 0, cz = 0;
    mat_trans_single(cx, cy, cz);
    if (cz < -mesh->radius) return; 

    // Dynamic LOD Switch
    if (cz > 300.0f && mesh->lod_fallback != NULL) {
        frog_mesh_render(mesh->lod_fallback, filter);
        return;
    }

    // Evaluate active contract
    int current_contract = (mesh->contract_override != -1) ? mesh->contract_override : (int)frog_engine_config.active_contract;

    // --- [ STEP 2: HARDWARE CONTEXT ] ---
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    
    int list;
    if (mesh->is_punch_thru) {
        list = PVR_LIST_PT_POLY; 
    } else {
        list = mesh->is_transparent ? PVR_LIST_TR_POLY : PVR_LIST_OP_POLY;
    }
    
    // Force-override texture filter configuration to BILINEAR if Glide contract is active
    int filter_mode = (current_contract == CONTRACT_GLIDE) ? PVR_FILTER_BILINEAR : (filter ? PVR_FILTER_BILINEAR : PVR_FILTER_NONE);

    pvr_poly_cxt_txr(&cxt, list, mesh->texture->format, mesh->texture->width, 
                     mesh->texture->height, mesh->texture->vram_ptr, 
                     filter_mode);
    
    // Overwrite internal texture register with custom baked mipmap bias.
    // If the Glide contract is active, we force a negative mipmap bias to mimic the Voodoo 5 5500.
    if (current_contract == CONTRACT_GLIDE) {
        cxt.txr.mipmap_bias = PVR_MIPBIAS_0_50; // Forces sharp texture transitions at distance (-0.5 offset)
    } else {
        cxt.txr.mipmap_bias = mesh->mipmap_bias;
    }

    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    float mesh_energy = world_clock_get_modulated(&wc, MOD_DEST_MESH_DISPLACEMENT);
    float dynamic_grid = 1.0f + (mesh_energy * 14.0f);
    float inv_grid = 1.0f / dynamic_grid; 

    // HOIST STATE CHANGES: Load the color grading matrix once prior to starting loops
    frog_color_prepare_vibe(current_contract);

    // --- [ STEP 3: OIT SORTING PASS & THE GEOMETRY FIREHOSE ] ---
    
    if (mesh->is_transparent && !mesh->is_punch_thru) {
        uint32_t face_count = mesh->vertex_count / 3;
        if (face_count == 0) return;

        FrogFace *faces;
        FrogFace *temp_faces;

        if (face_count <= MAX_SORT_FACES) {
            faces = sort_buffer_primary;
            temp_faces = sort_buffer_secondary;
        } else {
            // Allocate safely from Scratchpad to prevent mid-frame heap leaks
            size_t req_size = face_count * sizeof(FrogFace);
            faces = (FrogFace *)frog_scratchpad_alloc(req_size);
            temp_faces = (FrogFace *)frog_scratchpad_alloc(req_size);

            if (!faces || !temp_faces) {
                face_count = MAX_SORT_FACES;
                faces = sort_buffer_primary;
                temp_faces = sort_buffer_secondary;
            }
        }

        uint32_t i;
        for (i = 0; i < face_count; i++) {
            uint32_t v_idx = i * 3;
            faces[i].v[0] = mesh->vertices[v_idx];
            faces[i].v[1] = mesh->vertices[v_idx + 1];
            faces[i].v[2] = mesh->vertices[v_idx + 2];

            float x = faces[i].v[0].x, y = faces[i].v[0].y, z = faces[i].v[0].z;
            mat_trans_single(x, y, z);
            faces[i].depth = z; 
        }

        radix_sort_faces(faces, temp_faces, face_count);

        volatile uint32_t *sq_dest = (volatile uint32_t *)0xe0000000;

        for (i = 0; i < face_count; i++) {
            // SH-4 Instruction cache-prefetching the next triangle face structure
            if (i < face_count - 1) {
                __asm__ __volatile__("pref @%0" : : "r"(&faces[i + 1]));
            }

            int j;
            for (j = 0; j < 3; j++) {
                FrogVertex v = faces[i].v[j];

                float nx = v.x, ny = v.y, nz = v.z;
                mat_trans_single(nx, ny, nz);

                if (nz > 10.0f || nz < 0.0f) nz = 10.0f; 

                if (current_contract == CONTRACT_MIPS) {
                    nx = roundf(nx);
                    ny = roundf(ny);
                } else if (current_contract == CONTRACT_PARADOX && mesh_energy > 0.15f) {
                    int32_t temp_x = (int32_t)(nx * inv_grid);
                    int32_t temp_y = (int32_t)(ny * inv_grid);
                    nx = (float)temp_x * dynamic_grid;
                    ny = (float)temp_y * dynamic_grid;
                }

                pvr_vertex_t vert;
                vert.flags = (j == 2) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
                vert.x = nx; vert.y = ny; 
                vert.z = (current_contract == CONTRACT_MIPS) ? ((cz > 0.1f) ? cz : 0.1f) : nz;
                vert.u = v.u; vert.v = v.v;
                vert.argb = frog_color_apply_vibe_optimized(v.argb, nz, &wc);
                vert.oargb = v.oargb;

                uint32_t *src_ptr = (uint32_t *)&vert;
                sq_dest[0] = src_ptr[0]; sq_dest[1] = src_ptr[1];
                sq_dest[2] = src_ptr[2]; sq_dest[3] = src_ptr[3];
                sq_dest[4] = src_ptr[4]; sq_dest[5] = src_ptr[5];
                sq_dest[6] = src_ptr[6]; sq_dest[7] = src_ptr[7];

                __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));
            }
        }
    } else {
        volatile uint32_t *sq_dest = (volatile uint32_t *)0xe0000000;
        uint32_t i;
        for (i = 0; i < mesh->vertex_count; i++) {
            // SH-4 Prefetch instruction loading the next sequential vertex data
            if (i < mesh->vertex_count - 1) {
                __asm__ __volatile__("pref @%0" : : "r"(&mesh->vertices[i + 1]));
            }

            FrogVertex v = mesh->vertices[i];

            float nx = v.x, ny = v.y, nz = v.z;
            mat_trans_single(nx, ny, nz);

            if (nz > 10.0f || nz < 0.0f) nz = 10.0f;

            if (current_contract == CONTRACT_MIPS) {
                nx = roundf(nx);
                ny = roundf(ny);
            } else if (current_contract == CONTRACT_PARADOX && mesh_energy > 0.15f) {
                int32_t temp_x = (int32_t)(nx * inv_grid);
                int32_t temp_y = (int32_t)(ny * inv_grid);
                nx = (float)temp_x * dynamic_grid;
                ny = (float)temp_y * dynamic_grid;
            }

            pvr_vertex_t vert;
            vert.flags = (i == mesh->vertex_count - 1) ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            vert.x = nx; vert.y = ny; 
            vert.z = (current_contract == CONTRACT_MIPS) ? ((cz > 0.1f) ? cz : 0.1f) : nz;
            vert.u = v.u; vert.v = v.v;
            vert.argb = frog_color_apply_vibe_optimized(v.argb, nz, &wc);
            vert.oargb = v.oargb;

            uint32_t *src_ptr = (uint32_t *)&vert;
            sq_dest[0] = src_ptr[0]; sq_dest[1] = src_ptr[1];
            sq_dest[2] = src_ptr[2]; sq_dest[3] = src_ptr[3];
            sq_dest[4] = src_ptr[4]; sq_dest[5] = src_ptr[5];
            sq_dest[6] = src_ptr[6]; sq_dest[7] = src_ptr[7];

            __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));
        }
    }
}

void frog_mesh_unload(FrogMesh *mesh) {
    if(mesh) {
        if(mesh->vertices) free(mesh->vertices);
        free(mesh);
    }
}
