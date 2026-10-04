#ifndef HOST_COMPAT_PVR_H
#define HOST_COMPAT_PVR_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef void* pvr_ptr_t;
typedef uint32_t pvr_dr_state_t;

// Complete mock definitions of PVR compile pipelines
typedef struct {
    uint32_t flags;
    float x, y, z;
    float u, v;
    uint32_t argb;
    uint32_t oargb;
} pvr_vertex_t;

typedef struct {
    uint32_t val[8];
} pvr_poly_hdr_t;

typedef struct {
    uint32_t val[8];
} pvr_mod_hdr_t;

typedef struct {
    struct {
        int fog_type;
        int specular;
        int color_clamp;
    } gen;
    struct {
        int env;
    } txr;
    struct {
        int src;
        int dst;
    } blend;
} pvr_poly_cxt_t;

// PVR Compilation Flags
#define PVR_LIST_OP_POLY 0
#define PVR_LIST_PT_POLY 1
#define PVR_LIST_TR_POLY 2
#define PVR_LIST_OP_MOD  3

#define PVR_TXRFMT_RGB565   0
#define PVR_TXRFMT_ARGB4444 1

#define PVR_FILTER_BILINEAR 0
#define PVR_FILTER_NONE     1

#define PVR_FOG_TABLE 0
#define PVR_SPECULAR_ENABLE 0
#define PVR_TXRENV_DECAL 0
#define PVR_CLRCLAMP_ENABLE 0
#define PVR_BLEND_ONE 0
#define PVR_MODIFIER_NORMAL 0
#define PVR_CULLING_NONE 0
#define PVR_PAL_ARGB8888 0

extern uint8_t host_virtual_sq_buffer[1024 * 1024]; 
#define PVR_SQ_ADDR ((uintptr_t)host_virtual_sq_buffer)
#define PVR_TA_INPUT PVR_SQ_ADDR

static inline void pvr_prim(const void *item, size_t size) {
    (void)item; (void)size;
}

static inline void* pvr_dr_target(pvr_dr_state_t state) {
    (void)state;
    return (void*)host_virtual_sq_buffer;
}

static inline void pvr_dr_commit(const void *addr) {
    (void)addr;
}

static inline void pvr_poly_cxt_txr(pvr_poly_cxt_t *cxt, int list, int fmt, int w, int h, pvr_ptr_t ptr, int filter) {
    (void)cxt; (void)list; (void)fmt; (void)w; (void)h; (void)ptr; (void)filter;
}

static inline void pvr_poly_cxt_col(pvr_poly_cxt_t *cxt, int list) {
    (void)cxt; (void)list;
}

static inline void pvr_poly_compile(pvr_poly_hdr_t *hdr, pvr_poly_cxt_t *cxt) {
    (void)hdr; (void)cxt;
}

static inline void pvr_mod_compile(pvr_mod_hdr_t *hdr, int list, int mode, int culling) {
    (void)hdr; (void)list; (void)mode; (void)culling;
}

static inline pvr_ptr_t pvr_mem_malloc(size_t size) {
    return malloc(size);
}

static inline void pvr_mem_free(pvr_ptr_t ptr) {
    free(ptr);
}

static inline void pvr_set_pal_format(int format) { 
    (void)format; 
}

static inline void pvr_set_pal_entry(int idx, uint32_t color) { 
    (void)idx; (void)color; 
}

static inline void pvr_txr_load(const void *src, pvr_ptr_t dst, size_t size) {
    memcpy(dst, src, size);
}

#endif
