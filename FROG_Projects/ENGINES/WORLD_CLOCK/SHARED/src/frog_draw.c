//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez
#include <kos.h>
#include <dc/pvr.h>
#include <dc/sq.h>
#include <dc/video.h>
#include <dc/biosfont.h>
#include "frog_draw.h"
#include "frog_asset.h"
#include "frog_util.h"
#include "world_clock.h"

static pvr_poly_hdr_t h_sonic, h_saturn, h_nemo, h_toon, h_denim, h_neon, h_flat, h_flat_tr;

void frog_draw_init() {
    pvr_init_params_t params = {
        { PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16, PVR_BINSIZE_16 },
        1024 * 1024 
    };
    pvr_init(&params);

    pvr_fog_table_linear(10.0f, 100.0f);
    pvr_fog_table_color(1.0f, 0.8f, 0.6f, 0.4f);

    pvr_poly_cxt_t cxt;

    // 1. SONIC (Smooth 3D)
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 512, 512, 0, PVR_FILTER_BILINEAR);
    cxt.gen.fog_type = PVR_FOG_TABLE;
    pvr_poly_compile(&h_sonic, &cxt);

    // 2. SATURN (Sharp 2D)
    pvr_poly_cxt_txr(&cxt, PVR_LIST_PT_POLY, PVR_TXRFMT_ARGB4444, 512, 512, 0, PVR_FILTER_NONE);
    pvr_poly_compile(&h_saturn, &cxt);

    // 3. NEMO (Specular Glow)
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 512, 512, 0, PVR_FILTER_BILINEAR);
    cxt.gen.specular = PVR_SPECULAR_ENABLE;
    cxt.gen.fog_type = PVR_FOG_TABLE;
    pvr_poly_compile(&h_nemo, &cxt);

    // 4. TOON (Cel-Shaded)
    pvr_poly_cxt_txr(&cxt, PVR_LIST_PT_POLY, PVR_TXRFMT_ARGB4444, 512, 512, 0, PVR_FILTER_NONE);
    cxt.txr.env = PVR_TXRENV_DECAL; 
    pvr_poly_compile(&h_toon, &cxt);

    // 5. DENIM (Heavy Matte)
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_RGB565, 512, 512, 0, PVR_FILTER_BILINEAR);
    cxt.gen.color_clamp = PVR_CLRCLAMP_ENABLE; 
    pvr_poly_compile(&h_denim, &cxt);

    // 6. NEON (Additive Glow)
    pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB4444, 512, 512, 0, PVR_FILTER_BILINEAR);
    cxt.blend.src = PVR_BLEND_ONE;
    cxt.blend.dst = PVR_BLEND_ONE;
    pvr_poly_compile(&h_neon, &cxt);

    // 7. FLAT (UI/Gauges)
    pvr_poly_cxt_col(&cxt, PVR_LIST_OP_POLY);
    pvr_poly_compile(&h_flat, &cxt);

    // 8. FLAT (Translucent Overlay)
    pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
    pvr_poly_compile(&h_flat_tr, &cxt);

    frog_util_log("VIDEO: Hardware Signal Rails Forged.");
}

// Optimized rapid sprite submissions loading registers and writing directly to Store Queues
void frog_draw_sprite_rapid(float x, float y, float w, float h, frog_texture_t *tex, struct WorldClock *wc, uint32_t color, FrogVibe vibe) {
    if(!tex) return;

    pvr_poly_hdr_t *hdr;
    switch(vibe) {
        case VIBE_SATURN: hdr = &h_saturn; break;
        case VIBE_NEMO:   hdr = &h_nemo;   break;
        case VIBE_TOON:   hdr = &h_toon;   break;
        case VIBE_DENIM:  hdr = &h_denim;  break;
        case VIBE_NEON:   hdr = &h_neon;   break;
        default:          hdr = &h_sonic;  break;
    }

    pvr_prim(hdr, sizeof(pvr_poly_hdr_t));

    float energy = (wc) ? wc->sub_bass : 0.0f;
    uint32_t glow = (vibe == VIBE_NEMO || vibe == VIBE_NEON) ? 
                     PVR_PACK_COLOR(1.0f, 0.0f, energy, energy) : 0;

    volatile uint32_t *sq_dest = (volatile uint32_t *)0xe0000000;
    pvr_vertex_t vert;

    // Corner 1
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x; vert.y = y; vert.z = 1.0f; vert.u = 0.0f; vert.v = 0.0f;
    vert.argb = color; vert.oargb = glow;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    // Corner 2
    vert.x = x + w;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    // Corner 3
    vert.x = x; vert.y = y + h;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    // Corner 4 (EOL)
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x + w;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));
}

// Optimized UV sprite blitting using direct unrolled Store Queue writes
void frog_draw_sprite_uv(float x, float y, float w, float h, 
                         float u0, float v0, float u1, float v1,
                         frog_texture_t *tex, struct WorldClock *wc, 
                         uint32_t color, FrogVibe vibe) {
    if(!tex) return;

    pvr_poly_hdr_t *hdr;
    switch(vibe) {
        case VIBE_SATURN: hdr = &h_saturn; break;
        case VIBE_NEMO:   hdr = &h_nemo;   break;
        case VIBE_TOON:   hdr = &h_toon;   break;
        case VIBE_DENIM:  hdr = &h_denim;  break;
        case VIBE_NEON:   hdr = &h_neon;   break;
        default:          hdr = &h_sonic;  break;
    }

    pvr_prim(hdr, sizeof(pvr_poly_hdr_t));

    float energy = (wc) ? wc->sub_bass : 0.0f;
    uint32_t glow = (vibe == VIBE_NEMO || vibe == VIBE_NEON) ? 
                     PVR_PACK_COLOR(1.0f, 0.0f, energy, energy) : 0;

    volatile uint32_t *sq_dest = (volatile uint32_t *)0xe0000000;
    pvr_vertex_t vert;

    // Corner 1
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x; vert.y = y; vert.z = 1.0f; vert.u = u0; vert.v = v0;
    vert.argb = color; vert.oargb = glow;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    // Corner 2
    vert.x = x + w; vert.u = u1;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    // Corner 3
    vert.x = x; vert.y = y + h; vert.u = u0; vert.v = v1;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    // Corner 4 (EOL)
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x + w; vert.u = u1;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));
}

void frog_draw_saturn_quad(float x0, float y0, float x1, float y1,
                           float x2, float y2, float x3, float y3,
                           frog_texture_t *tex, uint32_t color) {
    if (!tex) return;

    pvr_prim(&h_saturn, sizeof(pvr_poly_hdr_t));

    volatile uint32_t *sq_dest = (volatile uint32_t *)0xe0000000;
    pvr_vertex_t vert;

    vert.flags = PVR_CMD_VERTEX;
    vert.argb = color; vert.oargb = 0; vert.z = 1.0f;

    vert.x = x0; vert.y = y0; vert.u = 0.0f; vert.v = 0.0f;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    vert.x = x1; vert.y = y1; vert.u = 1.0f; vert.v = 0.0f;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    vert.x = x2; vert.y = y2; vert.u = 0.0f; vert.v = 1.0f;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x3; vert.y = y3; vert.u = 1.0f; vert.v = 1.0f;
    memcpy((void *)sq_dest, &vert, 32);
    __asm__ __volatile__("pref @%0" : : "r"(0xe0000000));
}

void frog_draw_rect(float x, float y, float w, float h, uint32_t color) {
    pvr_prim(&h_flat, sizeof(pvr_poly_hdr_t));
    pvr_vertex_t vert;
    vert.z = 1.0f; vert.argb = color; vert.oargb = 0; vert.u = 0; vert.v = 0;
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;     vert.y = y + h; pvr_prim(&vert, sizeof(vert));
    vert.x = x;     vert.y = y;     pvr_prim(&vert, sizeof(vert));
    vert.x = x + w; vert.y = y + h; pvr_prim(&vert, sizeof(vert));
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x + w; vert.y = y;     pvr_prim(&vert, sizeof(vert));
}

void frog_draw_rect_tr(float x, float y, float w, float h, uint32_t color) {
    pvr_prim(&h_flat_tr, sizeof(pvr_poly_hdr_t));
    pvr_vertex_t vert;
    vert.z = 1.0f; vert.argb = color; vert.oargb = 0; vert.u = 0; vert.v = 0;
    vert.flags = PVR_CMD_VERTEX;
    vert.x = x;     vert.y = y + h; pvr_prim(&vert, sizeof(vert));
    vert.x = x;     vert.y = y;     pvr_prim(&vert, sizeof(vert));
    vert.x = x + w; vert.y = y + h; pvr_prim(&vert, sizeof(vert));
    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = x + w; vert.y = y;     pvr_prim(&vert, sizeof(vert));
}

void frog_draw_text(float x, float y, const char *text) {
    bfont_draw_str(vram_s + ((int)y * 640) + (int)x, 640, 0, text);
}
