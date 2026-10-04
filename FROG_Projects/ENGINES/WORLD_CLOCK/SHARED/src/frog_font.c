#include "frog_font.h"
#include "frog_draw.h"
#include "frog_asset.h"
#include "world_clock.h"
#include <string.h>

frog_texture_t* frog_font_load(const char *texture_path) {
    return frog_asset_load_pvr(texture_path);
}

void frog_font_print(float x, float y, float scale, uint32_t color, 
                     frog_texture_t *font_tex, struct WorldClock *wc, 
                     const char *text) {
    if(!font_tex || !text) return;

    int len = strlen(text);
    float current_x = x;
    const float char_size = 16.0f; 
    const float uv_unit = 0.0625f; // 1.0 / 16.0 (256-char ASCII standard 16x16 grid)

    for(int i = 0; i < len; i++) {
        char c = text[i];
        
        // Calculate the UV coordinate offsets inside the font sheet
        float u0 = (float)(c % 16) * uv_unit;
        float v0 = (float)(c / 16) * uv_unit;
        float u1 = u0 + uv_unit;
        float v1 = v0 + uv_unit;
        
        // Shunt the individual character slice to the PowerVR Tile Accelerator
        frog_draw_sprite_uv(
            current_x, y, 
            char_size * scale, 
            char_size * scale, 
            u0, v0, u1, v1,
            font_tex, 
            wc, 
            color, 
            VIBE_SATURN 
        );

        current_x += (char_size * scale) * 0.75f; 
    }
}

void frog_font_unload(frog_texture_t *font_tex) {
    frog_asset_unload(font_tex);
}
