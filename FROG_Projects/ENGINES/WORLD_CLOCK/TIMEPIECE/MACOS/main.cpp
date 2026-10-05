#define _LIBCPP_NO_EXPERIMENTAL_DEPRECATION_WARNING_FILESYSTEM 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <ctime>

#include "timepiece_assets.h"
#include "timepiece_clock.h"

// --- [ COUPLING & LINKAGE RESOLUTION ] ---
extern "C" {
    // Instantiates global variables with unmangled C linkage
    uint8_t host_virtual_sq_buffer[1024 * 1024];
    uint16_t host_vram_s[640 * 480];
    host_xmtrx_t host_xmtrx;
    WorldClock wc;

    uint32_t frog_audio_get_envelope_vol(uint32_t channel) {
        (void)channel; return 0;
    }

    uint32_t frog_audio_get_position(uint32_t channel) {
        (void)channel;
        static uint32_t dummy_playhead = 0;
        dummy_playhead = (dummy_playhead + 735) % 65536;
        return dummy_playhead;
    }

    void frog_audio_set_pitch(uint32_t channel, float pitch_multiplier) {
        (void)channel; (void)pitch_multiplier;
    }

    void frog_audio_trigger_momentum(uint32_t channel, uint32_t decay_rate) {
        (void)channel; (void)decay_rate;
    }
}

// --- [ WORKSPACE VIEWS CONFIGURATION ] ---
enum RenderMode { RENDER_WIREFRAME, RENDER_FLAT, RENDER_TEXTURED };
RenderMode g_view_mode[4] = { RENDER_WIREFRAME, RENDER_WIREFRAME, RENDER_WIREFRAME, RENDER_WIREFRAME };

// --- [ MATH, SELECTION & NAVIGATION HELPERS ] ---

float SnapToGrid(float val, float grid) {
    if (!g_grid_snap) return val;
    return roundf(val / grid) * grid;
}

bool IntersectRayAABB(vec3_t origin, vec3_t dir, frog_aabb_t aabb, float& t_out) {
    float t1 = (aabb.min.x - origin.x) / (dir.x + 1e-6f);
    float t2 = (aabb.max.x - origin.x) / (dir.x + 1e-6f);
    float t3 = (aabb.min.y - origin.y) / (dir.y + 1e-6f);
    float t4 = (aabb.max.y - origin.y) / (dir.y + 1e-6f);
    float t5 = (aabb.min.z - origin.z) / (dir.z + 1e-6f);
    float t6 = (aabb.max.z - origin.z) / (dir.z + 1e-6f);

    float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));

    if (tmax < 0 || tmin > tmax) return false;
    t_out = tmin;
    return true;
}

void GetScreenRay(float mouse_x, float mouse_y, float width, float height, vec3_t& out_origin, vec3_t& out_dir) {
    float norm_x = (2.0f * mouse_x) / width - 1.0f;
    float norm_y = 1.0f - (2.0f * mouse_y) / height;

    float yaw = g_cam_rot.y * 3.14159f / 180.0f;
    float pitch = g_cam_rot.x * 3.14159f / 180.0f;

    vec3_t forward = {
        sinf(yaw) * cosf(pitch),
        -sinf(pitch),
        -cosf(yaw) * cosf(pitch)
    };

    vec3_t right = { cosf(yaw), 0.0f, sinf(yaw) };
    vec3_t up = vec3_cross(right, forward);

    float fov_factor = tanf(60.0f / 360.0f * 3.14159f);
    float aspect = width / height;

    vec3_t ray_dir = vec3_add(
        vec3_scale(forward, 1.0f),
        vec3_add(
            vec3_scale(right, norm_x * fov_factor * aspect),
            vec3_scale(up, norm_y * fov_factor)
        )
    );

    out_origin = g_cam_pos;
    out_dir = vec3_normalize(ray_dir);
}

void HandleCameraControls(ImVec2 size) {
    ImGuiIO& io = ImGui::GetIO();
    if (!ImGui::IsWindowHovered()) return;

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        g_cam_rot.y += io.MouseDelta.x * 0.25f;
        g_cam_rot.x += io.MouseDelta.y * 0.25f;
        g_cam_rot.x = fmaxf(-89.0f, fminf(89.0f, g_cam_rot.x));
    }

    float speed = 1.0f;
    float yaw_rad = g_cam_rot.y * 3.14159f / 180.0f;
    vec3_t forward = { sinf(yaw_rad), 0.0f, -cosf(yaw_rad) };
    vec3_t right = { cosf(yaw_rad), 0.0f, sinf(yaw_rad) };

    if (ImGui::IsKeyDown(ImGuiKey_W)) g_cam_pos = vec3_add(g_cam_pos, vec3_scale(forward, speed));
    if (ImGui::IsKeyDown(ImGuiKey_S)) g_cam_pos = vec3_add(g_cam_pos, vec3_scale(forward, -speed));
    if (ImGui::IsKeyDown(ImGuiKey_D)) g_cam_pos = vec3_add(g_cam_pos, vec3_scale(right, speed));
    if (ImGui::IsKeyDown(ImGuiKey_A)) g_cam_pos = vec3_add(g_cam_pos, vec3_scale(right, -speed));
}

void HandleOrthoNavigation(int view_id, ImVec2 size) {
    ImGuiIO& io = ImGui::GetIO();
    if (!ImGui::IsWindowHovered()) return;

    if (io.MouseWheel != 0.0f) {
        g_ortho_zoom[view_id] -= io.MouseWheel * 0.08f;
        g_ortho_zoom[view_id] = fmaxf(0.05f, fminf(10.0f, g_ortho_zoom[view_id]));
    }

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        float scale = g_ortho_zoom[view_id] * (100.0f / size.y);
        g_ortho_pan[view_id].x += io.MouseDelta.x * scale;
        g_ortho_pan[view_id].y -= io.MouseDelta.y * scale;
    }
}

// --- [ DRAWING HELPERS ] ---

void DrawCelestialGrids(float step, float range) {
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    for (float i = -range; i <= range; i += step) {
        if (fabsf(i) < 0.001f) {
            glColor3f(0.83f, 0.71f, 0.23f); // Gold Primary axis
        } else {
            glColor3f(0.04f, 0.20f, 0.14f); // Faint emerald green
        }
        glVertex3f(i, -range, 0.0f); glVertex3f(i, range, 0.0f);
        glVertex3f(-range, i, 0.0f); glVertex3f(range, i, 0.0f);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    glColor4f(0.09f, 0.55f, 0.38f, 0.3f);
    for (int a = 0; a <= 360; a += 10) {
        float r = (float)a * 3.14159f / 180.0f;
        glVertex3f(cosf(r) * 48.0f, sinf(r) * 48.0f, 0.0f);
    }
    glEnd();
}

void DrawWireframeAABB(const frog_aabb_t& aabb, uint32_t color) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8)  & 0xFF) / 255.0f;
    float b = ( color        & 0xFF) / 255.0f;
    glColor3f(r, g, b);

    glBegin(GL_LINES);
    glVertex3f(aabb.min.x, aabb.min.y, aabb.min.z); glVertex3f(aabb.max.x, aabb.min.y, aabb.min.z);
    glVertex3f(aabb.max.x, aabb.min.y, aabb.min.z); glVertex3f(aabb.max.x, aabb.min.y, aabb.max.z);
    glVertex3f(aabb.max.x, aabb.min.y, aabb.max.z); glVertex3f(aabb.min.x, aabb.min.y, aabb.max.z);
    glVertex3f(aabb.min.x, aabb.min.y, aabb.max.z); glVertex3f(aabb.min.x, aabb.min.y, aabb.min.z);

    glVertex3f(aabb.min.x, aabb.max.y, aabb.min.z); glVertex3f(aabb.max.x, aabb.max.y, aabb.min.z);
    glVertex3f(aabb.max.x, aabb.max.y, aabb.min.z); glVertex3f(aabb.max.x, aabb.max.y, aabb.max.z);
    glVertex3f(aabb.max.x, aabb.max.y, aabb.max.z); glVertex3f(aabb.min.x, fmaxf(aabb.min.y, aabb.max.y), aabb.max.z);
    glVertex3f(aabb.min.x, aabb.max.y, aabb.max.z); glVertex3f(aabb.min.x, aabb.max.y, aabb.min.z);

    glVertex3f(aabb.min.x, aabb.min.y, aabb.min.z); glVertex3f(aabb.min.x, aabb.max.y, aabb.min.z);
    glVertex3f(aabb.max.x, aabb.min.y, aabb.min.z); glVertex3f(aabb.max.x, aabb.max.y, aabb.min.z);
    glVertex3f(aabb.max.x, aabb.min.y, aabb.max.z); glVertex3f(aabb.max.x, aabb.max.y, aabb.max.z);
    glVertex3f(aabb.min.x, aabb.min.y, aabb.max.z); glVertex3f(aabb.min.x, aabb.max.y, aabb.max.z);
    glEnd();
}

// --- [ VIEWPORT RENDERING PIPELINE ] ---
void RenderViewport(int view_id, int x, int y, int width, int height) {
    glViewport(x, y, width, height);
    glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    if (g_view_mode[view_id] == RENDER_WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (height > 0) ? ((float)width / (float)height) : 1.0f;

    if (view_id == 0) { // 3D Viewport
        float fH = tanf(60.0f / 360.0f * 3.14159f) * 0.1f;
        float fW = fH * aspect;
        glFrustum(-fW, fW, -fH, fH, 0.1f, 1000.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(g_cam_rot.x, 1.0f, 0.0f, 0.0f);
        glRotatef(g_cam_rot.y, 0.0f, 1.0f, 0.0f);
        glTranslatef(-g_cam_pos.x, -g_cam_pos.y, -g_cam_pos.z);
        glEnable(GL_DEPTH_TEST);

        glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        DrawCelestialGrids(g_grid_units, 256.0f);
        glPopMatrix();
    } 
    else { // 2D Orthographic Views
        glDisable(GL_DEPTH_TEST);
        float size = 50.0f * g_ortho_zoom[view_id - 1];
        glOrtho(-size * aspect, size * aspect, -size, size, -1000.0f, 1000.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(g_ortho_pan[view_id - 1].x, g_ortho_pan[view_id - 1].y, 0.0f);

        if (view_id == 1) { // Top View (X/Z)
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        } else if (view_id == 3) { // Side View (Z/Y)
            glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        }

        DrawCelestialGrids(g_grid_units, 256.0f);
    }

    for (size_t i = 0; i < g_brushes.size(); ++i) {
        uint32_t color = g_brushes[i].color;
        if (g_selected_brush == (int)i) color = 0xFFFF0000;
        DrawWireframeAABB(g_brushes[i].bounds, color);
    }

    for (size_t i = 0; i < g_entities.size(); ++i) {
        const auto& ent = g_entities[i];
        frog_aabb_t bounds = {
            { ent.position.x - 1.5f, ent.position.y - 1.5f, ent.position.z - 1.5f },
            { ent.position.x + 1.5f, ent.position.y + 1.5f, ent.position.z + 1.5f }
        };
        uint32_t color = (g_selected_entity == (int)i) ? 0xFFFF0000 : 0xFFFFFF00;
        DrawWireframeAABB(bounds, color);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// --- [ ACTIVE DIRECT-LOADING VIEWPORT SPLASH RENDERER ] ---
void RenderActiveSplashScreen(ImGuiIO& io) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::Begin("SplashWindow", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (g_splash_texture != 0) {
        float img_aspect = (float)g_splash_width / (float)g_splash_height;
        float vp_aspect = viewport->Size.x / viewport->Size.y;
        
        ImVec2 p_min = viewport->Pos;
        ImVec2 p_max = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);

        if (vp_aspect > img_aspect) {
            float w = viewport->Size.y * img_aspect;
            float x_offset = (viewport->Size.x - w) / 2.0f;
            p_min.x += x_offset;
            p_max.x = p_min.x + w;
        } else {
            float h = viewport->Size.x / img_aspect;
            float y_offset = (viewport->Size.y - h) / 2.0f;
            p_min.y += y_offset;
            p_max.y = p_min.y + h;
        }
        
        draw_list->AddImage((ImTextureID)(uintptr_t)g_splash_texture, p_min, p_max);
    }

    ImVec2 center = ImVec2(viewport->Size.x / 2.0f, viewport->Size.y / 2.0f);
    float time_val = g_splash_timer * 1.5f;
    for (int i = 0; i < 2; i++) {
        float r = 240.0f + (i * 30.0f);
        draw_list->AddCircle(center, r, ImGui::GetColorU32(ImVec4(0.83f, 0.71f, 0.23f, 0.15f)), 64, 1.0f);
        
        float pt_x = center.x + cosf(time_val * (0.8f - i * 0.1f)) * r;
        float pt_y = center.y + sinf(time_val * (0.8f - i * 0.1f)) * r;
        draw_list->AddCircleFilled(ImVec2(pt_x, pt_y), 4.0f, ImGui::GetColorU32(ImVec4(0.09f, 0.55f, 0.38f, 0.50f)), 16);
    }

    ImVec2 loading_pos = ImVec2(40.0f, viewport->Size.y - 60.0f);
    
    draw_list->AddText(ImGui::GetFont(), 24.0f, loading_pos, 
                       ImGui::GetColorU32(ImVec4(0.2f, 0.9f, 0.3f, 0.90f)), "Loading...");

    ImGui::End();
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "TIMEPIECE - Level Editor (Sega Dreamcast)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1440, 810,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

#ifdef __APPLE__
    // --- [ FIXED: Target timepiece_darwin.icns explicitly for your window icon ] ---
    SDL_Surface* app_icon = LoadICNSAsSDLSurface("resources/timepiece_darwin.icns");
    if (app_icon) {
        SDL_SetWindowIcon(window, app_icon);
        SDL_FreeSurface(app_icon);
    }
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ApplyCosmicJadeTheme();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

#ifdef __APPLE__
    g_splash_texture = LoadPNGNativeMacOS("resources/timepiece_splash.png", &g_splash_width, &g_splash_height);
#endif

    g_brushes.push_back({ {{-40.0f, -4.0f, -40.0f}, {40.0f, 0.0f, 40.0f}}, 0xFF555555, "stone_base" });
    g_entities.push_back({ "info_player_start", {0, 2.0f, 0}, {0,0,0}, "spawn1", "" });

    world_clock_init(&wc);
    ScanGamesDirectory();

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_DROPFILE) {
                char* dropped_path = event.drop.file;
                std::string path_str(dropped_path);
                if (path_str.rfind(".tpc") == path_str.length() - 4) {
                    LoadTimePieceProject(path_str, g_brushes, g_entities);
                    g_app_state = STATE_EDITOR; 
                }
                SDL_free(dropped_path);
            }
        }

        world_clock_update(&wc);

        SimulateAudioTelemetry(wc.delta_time);

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (g_app_state == STATE_SPLASH_SCREEN) {
            g_splash_timer += io.DeltaTime;
            glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            RenderActiveSplashScreen(io);

            if (g_splash_timer > 3.5f) {
                if (g_splash_texture != 0) {
                    glDeleteTextures(1, &g_splash_texture);
                    g_splash_texture = 0;
                }
                g_app_state = STATE_LAUNCHER;
            }
        } 
        else if (g_app_state == STATE_LAUNCHER) {
            glClearColor(0.04f, 0.05f, 0.08f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ImGuiViewport* vp = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(vp->Size.x / 2.0f - 300.0f, vp->Size.y / 2.0f - 200.0f));
            ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f));
            
            ImGui::Begin("TIMEPIECE - Project Selector Launcher", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
            ImGui::Text("Active Hardware Pipeline Launcher");
            ImGui::Separator();

            ImGui::Columns(2, "launcher_split", true);
            
            ImGui::Text("Discovered Game Projects:");
            ImGui::BeginChild("scanned_games_child", ImVec2(0, 250.0f), true);
            for (const auto& game : g_scanned_games) {
                if (ImGui::Selectable(game.c_str(), g_active_game == game)) {
                    g_active_game = game;
                    g_active_game_path = "../../GAMES/" + game;
                }
            }
            ImGui::EndChild();

            if (!g_active_game.empty()) {
                if (ImGui::Button("Load Selected Game", ImVec2(-1, 0))) {
                    g_app_state = STATE_EDITOR; 
                }
            }

            ImGui::NextColumn();

            ImGui::Text("Create New Game Project:");
            static char new_game_name[64] = "";
            ImGui::InputText("Project Name", new_game_name, 64);
            if (ImGui::Button("Generate Structure", ImVec2(-1, 0))) {
                if (CreateNewGameProject(new_game_name)) {
                    g_active_game = new_game_name;
                    g_active_game_path = "../../GAMES/" + g_active_game;
                    g_app_state = STATE_EDITOR;
                }
            }
            ImGui::Separator();
            ImGui::TextWrapped("Dragging a '.tpc' project file directly onto this window will mount it.");

            ImGui::Columns(1);
            ImGui::End();
        } 
        else if (g_app_state == STATE_EDITOR) {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
            ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
            ImGui::Begin("TIMEPIECE_Base", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | 
                         ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | 
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
            
            ImGuiID dockspace_id = ImGui::GetID("BaseLayout");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Save Map to SHARED...")) {
                        ExportLevelBinary("../SHARED/level01.fmp", g_brushes, g_entities);
                        std::cout << "TIMEPIECE: Level successfully exported." << std::endl;
                    }
                    if (ImGui::MenuItem("Back to Launcher")) {
                        g_app_state = STATE_LAUNCHER;
                    }
                    if (ImGui::MenuItem("Exit")) { running = false; }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Grid")) {
                    ImGui::MenuItem("Snap Active", nullptr, &g_grid_snap);
                    if (ImGui::BeginMenu("Grid Resolution")) {
                        if (ImGui::MenuItem("4 Units", nullptr, g_grid_units == 4.0f)) g_grid_units = 4.0f;
                        if (ImGui::MenuItem("8 Units", nullptr, g_grid_units == 8.0f)) g_grid_units = 8.0f;
                        if (ImGui::MenuItem("16 Units", nullptr, g_grid_units == 16.0f)) g_grid_units = 16.0f;
                        if (ImGui::MenuItem("32 Units", nullptr, g_grid_units == 32.0f)) g_grid_units = 32.0f;
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();

            ImGui::Begin("Entity Directory");
            for (size_t i = 0; i < g_entities.size(); ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Entity %zu: %s", i, g_entities[i].classname);
                if (ImGui::Selectable(label, g_selected_entity == (int)i)) {
                    g_selected_entity = i;
                    g_selected_brush = -1;
                }
            }
            if (ImGui::Button("Spawn Point Entity")) {
                EditorEntity e = { "trigger_multiple", {0, 2.0f, 0}, {0,0,0}, "target_vol", "" };
                g_entities.push_back(e);
            }
            ImGui::End();

            ImGui::Begin("Solid Geometry Brushes");
            for (size_t i = 0; i < g_brushes.size(); ++i) {
                char label[64];
                snprintf(label, sizeof(label), "Brush %zu: (%s)", i, g_brushes[i].texture);
                if (ImGui::Selectable(label, g_selected_brush == (int)i)) {
                    g_selected_brush = i;
                    g_selected_entity = -1;
                }
            }
            if (ImGui::Button("Create Convex Brush")) {
                EditorBrush b = { {{-8.0f, 0.0f, -8.0f}, {8.0f, 16.0f, 8.0f}}, 0xFF4B7055, "jade_pillar" };
                g_brushes.push_back(b);
            }
            ImGui::End();

            ImGui::Begin("Selected Node Properties");
            if (g_selected_entity != -1) {
                auto& ent = g_entities[g_selected_entity];
                ImGui::InputText("Target Class", ent.classname, 32);
                if (ImGui::DragFloat3("Coord Position", &ent.position.x, 0.1f)) {
                    ent.position.x = SnapToGrid(ent.position.x, g_grid_units);
                    ent.position.y = SnapToGrid(ent.position.y, g_grid_units);
                    ent.position.z = SnapToGrid(ent.position.z, g_grid_units);
                }
                ImGui::InputText("Target Name", ent.targetname, 32);
                ImGui::InputText("Trigger Target", ent.target, 32);
            } 
            else if (g_selected_brush != -1) {
                auto& b = g_brushes[g_selected_brush];
                ImGui::Text("AABB Geometric Bounds");
                if (ImGui::DragFloat3("AABB Min", &b.bounds.min.x, 0.1f)) {
                    b.bounds.min.x = SnapToGrid(b.bounds.min.x, g_grid_units);
                    b.bounds.min.y = SnapToGrid(b.bounds.min.y, g_grid_units);
                    b.bounds.min.z = SnapToGrid(b.bounds.min.z, g_grid_units);
                }
                if (ImGui::DragFloat3("AABB Max", &b.bounds.max.x, 0.1f)) {
                    b.bounds.max.x = SnapToGrid(b.bounds.max.x, g_grid_units);
                    b.bounds.max.y = SnapToGrid(b.bounds.max.y, g_grid_units);
                    b.bounds.max.z = SnapToGrid(b.bounds.max.z, g_grid_units);
                }
                ImGui::InputText("Texture Key", b.texture, 32);
            }
            ImGui::End();

            ImGui::Begin("Chrono-Audio Deck");
            ImGui::Text("AICA SPU Wave Signal Telemetry");
            
            ImGui::ProgressBar(wc.sub_bass, ImVec2(-1, 0), "Sub-Bass (MOD_SRC_SUB_BASS)");
            ImGui::ProgressBar(wc.mids, ImVec2(-1, 0), "Mids (MOD_SRC_MIDS)");
            ImGui::ProgressBar(wc.treble, ImVec2(-1, 0), "Treble (MOD_SRC_TREBLE)");
            ImGui::ProgressBar(wc.mid_energy, ImVec2(-1, 0), "Stereo Mid-Center (MOD_SRC_MID_CENTER)");
            ImGui::ProgressBar(wc.side_energy, ImVec2(-1, 0), "Stereo Side-Width (MOD_SRC_SIDE_WIDTH)");
            
            ImGui::Text("SPU Panning Bias: %s", wc.pan_bias > 0.05f ? "Right Shift" : (wc.pan_bias < -0.05f ? "Left Shift" : "Mono Center"));
            ImGui::SliderFloat("Panning Trace", &wc.pan_bias, -1.0f, 1.0f, "Pan Bias: %.2f");

            ImGui::Separator();
            ImGui::Text("AICA Emulated Tape Transport Controls");
            if (g_audio_deck.is_playing) {
                if (ImGui::Button("Pause Deck")) {
                    g_audio_deck.is_playing = false;
                }
            } else {
                if (ImGui::Button("Play Deck")) {
                    g_audio_deck.is_playing = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Rewind")) {
                g_audio_deck.current_time = 0.0f;
            }

            ImGui::Text("Timeline Position: %.2f / %.2f seconds", g_audio_deck.current_time, g_audio_deck.duration);
            
            ImGui::Separator();
            ImGui::Text("Signal Modulation Routing Matrix");
            const char* src_names[] = { "Sub-Bass", "Mids", "Treble", "Master Peak", "Mid-Center", "Side-Width", "None" };
            const char* dest_names[] = { "Ambient Light Gain", "Stroke Jitter", "VMU LCD VU Bar", "Mesh Displacement" };

            for (int i = 0; i < MOD_DEST_COUNT; ++i) {
                int current_src = (int)g_mod_matrix[i];
                if (ImGui::Combo(dest_names[i], &current_src, src_names, 7)) {
                    g_mod_matrix[i] = (ModSource)current_src;
                }
            }
            ImGui::End();

            ImGui::Begin("Telemetry Gates (SPU Trigger Hysteresis)");
            for (int i = 0; i < 4; i++) {
                ImGui::PushID(i);
                char bar_label[64];
                snprintf(bar_label, sizeof(bar_label), "Gate [%d]: %s", i, wc.gates[i].is_open ? "OPEN (Active)" : "CLOSED");
                
                if (ImGui::CollapsingHeader(bar_label, ImGuiTreeNodeFlags_DefaultOpen)) {
                    int source_selection = (int)wc.gates[i].source;
                    if (ImGui::Combo("Signal Source", &source_selection, src_names, 7)) {
                        wc.gates[i].source = (ModSource)source_selection;
                    }
                    ImGui::SliderFloat("Threshold", &wc.gates[i].threshold, 0.0f, 1.0f, "Limit: %.2f");
                    ImGui::SliderFloat("Hysteresis", &wc.gates[i].hysteresis, 0.0f, 0.5f, "Delta: %.2f");
                    
                    ImGui::Text("Gate Status: ");
                    ImGui::SameLine();
                    if (wc.gates[i].is_open) {
                        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "OPEN (Trigger active)");
                    } else {
                        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "CLOSED");
                    }
                }
                ImGui::PopID();
            }
            ImGui::End();

            ImGui::Begin("Time-Stream Timeline");
            ImGui::Text("Chrono-Events (Sequence Map)");
            if (ImGui::Button("Add Sequence Event")) {
                g_entities.push_back({ "trigger_chrono_relay", {0, 0, 0}, {0,0,0}, "timeline_relay", "" });
            }
            
            ImGui::Separator();
            for (size_t i = 0; i < g_entities.size(); ++i) {
                char bar_label[64];
                snprintf(bar_label, sizeof(bar_label), "Seq %zu: %s", i, g_entities[i].classname);
                float position_ratio = (g_entities[i].position.x + 40.0f) / 80.0f; 
                ImGui::Text("%s", bar_label);
                ImGui::SameLine(150.0f);
                ImGui::PushID(i);
                ImGui::SliderFloat("##time_pos", &position_ratio, 0.0f, 1.0f, "Step: %.2f");
                g_entities[i].position.x = (position_ratio * 80.0f) - 40.0f; 
                ImGui::PopID();
            }
            ImGui::End();

            ImGui::Begin("Asset Compiler Pipeline");
            ImGui::Text("Compile Blender Models (.obj -> .fmg)");
            static char raw_model_path[128] = "raw_assets/jade_pillar.obj";
            static char comp_model_path[128] = "../../GAMES/my_game/romdisk/jade_pillar.fmg";
            ImGui::InputText("Source .obj", raw_model_path, 128);
            ImGui::InputText("Output .fmg", comp_model_path, 128);
            if (ImGui::Button("Compile 3D Model")) {
                CompileBlenderObjToFmg(raw_model_path, comp_model_path);
            }

            ImGui::Separator();
            ImGui::Text("Project Save / Load State");
            static char project_path[128] = "projects/level01.tpc";
            ImGui::InputText("Project File", project_path, 128);
            if (ImGui::Button("Save Current Project")) {
                SaveTimePieceProject(project_path, g_brushes, g_entities);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Project File")) {
                LoadTimePieceProject(project_path, g_brushes, g_entities);
            }
            ImGui::End();

            const char* pane_names[4] = { "Camera (3D Perspective)", "Top View (X / Z)", "Front View (X / Y)", "Side View (Z / Y)" };
            for (int i = 0; i < 4; ++i) {
                // Pass ImGuiWindowFlags_NoBackground so ImGui doesn't draw over your OpenGL lines
                ImGui::Begin(pane_names[i], nullptr, ImGuiWindowFlags_NoBackground);
                ImVec2 pane_size = ImGui::GetContentRegionAvail();
                ImVec2 pane_pos = ImGui::GetCursorScreenPos();

                if (i > 0) {
                    HandleOrthoNavigation(i - 1, pane_size);
                }

                if (i == 0) {
                    HandleCameraControls(pane_size);

                    if (ImGui::BeginPopupContextWindow()) {
                        if (ImGui::MenuItem("Create Jade Brush at Cursor")) {
                            EditorBrush b = { {{g_cam_pos.x - 4.0f, g_cam_pos.y - 4.0f, g_cam_pos.z - 15.0f},
                                               {g_cam_pos.x + 4.0f, g_cam_pos.y + 4.0f, g_cam_pos.z - 7.0f}}, 
                                              0xFF095526, "jade_block" };
                            g_brushes.push_back(b);
                        }
                        if (ImGui::MenuItem("Create Info Spawn at Cursor")) {
                            EditorEntity e = { "info_spawn", {g_cam_pos.x, g_cam_pos.y, g_cam_pos.z - 10.0f}, {0,0,0}, "target", "" };
                            g_entities.push_back(e);
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered()) {
                        ImVec2 m_pos = ImGui::GetMousePos();
                        ImVec2 w_pos = ImGui::GetWindowPos();
                        ImVec2 content_pos = ImGui::GetWindowContentRegionMin();
                        
                        float relative_x = m_pos.x - (w_pos.x + content_pos.x);
                        float relative_y = m_pos.y - (w_pos.y + content_pos.y);

                        vec3_t origin, dir;
                        GetScreenRay(relative_x, relative_y, pane_size.x, pane_size.y, origin, dir);

                        float closest_t = 999999.0f;
                        int picked_brush = -1;

                        for (size_t b_idx = 0; b_idx < g_brushes.size(); ++b_idx) {
                            float t_hit;
                            if (IntersectRayAABB(origin, dir, g_brushes[b_idx].bounds, t_hit)) {
                                if (t_hit < closest_t) {
                                    closest_t = t_hit;
                                    picked_brush = b_idx;
                                }
                            }
                        }

                        if (picked_brush != -1) {
                            g_selected_brush = picked_brush;
                            g_selected_entity = -1;
                        }
                    }
                }

                // --- [ SCREEN TO WORLD COUPLING SHUNT ] ---
                int gl_x = (int)pane_pos.x;
                int gl_y = (int)io.DisplaySize.y - (int)(pane_pos.y + pane_size.y);
                int gl_w = (int)pane_size.x;
                int gl_h = (int)pane_size.y;

                if (gl_w > 0 && gl_h > 0) {
                    glEnable(GL_SCISSOR_TEST);
                    glScissor(gl_x, gl_y, gl_w, gl_h);

                    RenderViewport(i, gl_x, gl_y, gl_w, gl_h);

                    glDisable(GL_SCISSOR_TEST);
                }

                ImGui::End();
            }
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
