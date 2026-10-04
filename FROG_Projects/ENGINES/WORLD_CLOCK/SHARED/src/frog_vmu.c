#include "frog_vmu.h"
#include <stdio.h>
#include <string.h>

// Find the first available VMU on any controller port
maple_device_t* frog_vmu_get_device() {
    maple_device_t *vmu = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
    if (!vmu) {
        printf("FROG_VMU: No VMU detected!\n");
    }
    return vmu;
}

// Draw a 48x32 monochrome icon to the VMU screen
void frog_vmu_draw_icon(const uint8_t *bitmap) {
    maple_device_t *vmu = frog_vmu_get_device();
    if (vmu) {
        vmu_draw_lcd(vmu, bitmap);
    }
}

// Save game data (High scores, lap times, unlocks)
int frog_vmu_save(const char *filename, void *data, int size) {
    char path[64];
    // For simplicity, we target the first VMU found (usually /vmu/a1)
    sprintf(path, "/vmu/a1/%s", filename);

    file_t f = fs_open(path, O_WRONLY | O_CREAT);
    if (f == FILEHND_INVALID) return -1;

    fs_write(f, data, size);
    fs_close(f);
    return 0;
}

// Load game data
int frog_vmu_load(const char *filename, void *data, int size) {
    char path[64];
    sprintf(path, "/vmu/a1/%s", filename);

    file_t f = fs_open(path, O_RDONLY);
    if (f == FILEHND_INVALID) return -1;

    fs_read(f, data, size);
    fs_close(f);
    return 0;
}
