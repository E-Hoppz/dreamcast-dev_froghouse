#ifndef TIMEPIECE_ASSETS_H
#define TIMEPIECE_ASSETS_H

#include <string>
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>

extern "C" {
    #include "frog_math_3d.h"
    #include "frog_collision_3d.h"
    #include "frog_mesh.h"
    #include "world_clock.h"
}

// State machine State
enum EditorState { STATE_SPLASH_SCREEN, STATE_LAUNCHER, STATE_EDITOR };

// Level editor data structures
struct EditorEntity {
    char classname[32];
    vec3_t position;
    vec3_t rotation;
    char targetname[32];
    char target[32];
};

struct EditorBrush {
    frog_aabb_t bounds;
    uint32_t color;
    char texture[32];
    int      contract; // -1 = Use Global, 0 = Opaque, 1 = Glide, 2 = MIPS, 3 = Paradox
};

struct TimelineEvent {
    float time_seconds;
    char event_type[32];
    char parameters[64];
};

// Global asset references
extern GLuint g_splash_texture;
extern int g_splash_width;
extern int g_splash_height;

extern EditorState g_app_state;
extern float g_splash_timer;

extern std::vector<std::string> g_scanned_games;
extern std::string g_active_game;
extern std::string g_active_game_path;

extern std::vector<EditorBrush> g_brushes;
extern std::vector<EditorEntity> g_entities;
extern std::vector<TimelineEvent> g_events;

extern int g_selected_entity;
extern int g_selected_brush;
extern int g_selected_event;

extern bool g_grid_snap;
extern float g_grid_units;

// --- [ FIXED: Added missing extern declarations for viewport navigation ] ---
extern float g_ortho_zoom[3];
extern vec3_t g_ortho_pan[3];

extern vec3_t g_cam_pos;
extern vec3_t g_cam_rot;

void ScanGamesDirectory();
bool CreateNewGameProject(const std::string& name);
void SaveTimePieceProject(const std::string& filepath, const std::vector<EditorBrush>& brushes, const std::vector<EditorEntity>& entities);
void LoadTimePieceProject(const std::string& filepath, std::vector<EditorBrush>& brushes, std::vector<EditorEntity>& entities);
bool CompileBlenderObjToFmg(const std::string& obj_filepath, const std::string& fmg_filepath);
void ExportLevelBinary(const char* filepath, const std::vector<EditorBrush>& brushes, const std::vector<EditorEntity>& entities);

// --- [ FIXED: Expose the custom layout theme configuration to main.cpp ] ---
void ApplyCosmicJadeTheme();

#ifdef __APPLE__
GLuint LoadPNGNativeMacOS(const char* filepath, int* out_width, int* out_height);
SDL_Surface* LoadICNSAsSDLSurface(const char* filepath);
#endif

#endif
