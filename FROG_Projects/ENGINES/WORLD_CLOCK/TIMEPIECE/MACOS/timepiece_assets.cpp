#define _LIBCPP_NO_EXPERIMENTAL_DEPRECATION_WARNING_FILESYSTEM 1
#include "imgui.h" // --- [ FIXED: Resolves undefined template ImGuiStyle / ImVec4 ] ---
#include "timepiece_assets.h"
#include "timepiece_clock.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

// Resolves std::filesystem namespace routing for both C++11 and C++17 standards on macOS
#if defined(__APPLE__) && __cplusplus < 201703L
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

// --- [ REDUNDANT STRUCTS DELETED: Already cleanly declared in timepiece_assets.h ] ---

// Instantiates global variables
std::vector<EditorBrush> g_brushes;
std::vector<EditorEntity> g_entities;
std::vector<TimelineEvent> g_events;

int g_selected_entity = -1;
int g_selected_brush = -1;
int g_selected_event = -1;

bool g_grid_snap = true;
float g_grid_units = 8.0f;

vec3_t g_cam_pos = { 0.0f, 20.0f, 50.0f, 1.0f };
vec3_t g_cam_rot = { 30.0f, -45.0f, 0.0f, 0.0f };

GLuint g_splash_texture = 0;
int g_splash_width = 0;
int g_splash_height = 0;

EditorState g_app_state = STATE_SPLASH_SCREEN;
float g_splash_timer = 0.0f;

std::vector<std::string> g_scanned_games;
std::string g_active_game = "";
std::string g_active_game_path = "";

// Instantiated the viewport arrays
float g_ortho_zoom[3] = { 1.0f, 1.0f, 1.0f };
vec3_t g_ortho_pan[3] = { {0,0,0}, {0,0,0}, {0,0,0} };

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

GLuint LoadPNGNativeMacOS(const char* filepath, int* out_width, int* out_height) {
    CFStringRef pathRef = CFStringCreateWithCString(NULL, filepath, kCFStringEncodingUTF8);
    CFURLRef urlRef = CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, false);
    CFRelease(pathRef);
    if (!urlRef) return 0;

    CGImageSourceRef sourceRef = CGImageSourceCreateWithURL(urlRef, NULL);
    CFRelease(urlRef);
    if (!sourceRef) return 0;

    CGImageRef imageRef = CGImageSourceCreateImageAtIndex(sourceRef, 0, NULL);
    CFRelease(sourceRef);
    if (!imageRef) return 0;

    size_t width = CGImageGetWidth(imageRef);
    size_t height = CGImageGetHeight(imageRef);
    *out_width = (int)width;
    *out_height = (int)height;

    std::vector<uint32_t> buffer(width * height);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef contextRef = CGBitmapContextCreate(
        buffer.data(), width, height, 8, width * 4, colorSpace,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    );
    CGColorSpaceRelease(colorSpace);

    if (!contextRef) {
        CGImageRelease(imageRef);
        return 0;
    }

  //  CGContextTranslateCTM(contextRef, 0, height);
   // CGContextScaleCTM(contextRef, 1.0, -1.0);

    CGContextDrawImage(contextRef, CGRectMake(0, 0, width, height), imageRef);
    CGContextRelease(contextRef);
    CGImageRelease(imageRef);

    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    return texture_id;
}

SDL_Surface* LoadICNSAsSDLSurface(const char* filepath) {
    CFStringRef pathRef = CFStringCreateWithCString(NULL, filepath, kCFStringEncodingUTF8);
    CFURLRef urlRef = CFURLCreateWithFileSystemPath(NULL, pathRef, kCFURLPOSIXPathStyle, false);
    CFRelease(pathRef);
    if (!urlRef) return nullptr;

    CGImageSourceRef sourceRef = CGImageSourceCreateWithURL(urlRef, NULL);
    CFRelease(urlRef);
    if (!sourceRef) return nullptr;

    // CGImageSource natively extracts the primary image index (usually 512x512 or 256x256) inside the .icns container
    CGImageRef imageRef = CGImageSourceCreateImageAtIndex(sourceRef, 0, NULL);
    CFRelease(sourceRef);
    if (!imageRef) return nullptr;

    size_t width = CGImageGetWidth(imageRef);
    size_t height = CGImageGetHeight(imageRef);

    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        CGImageRelease(imageRef);
        return nullptr;
    }

    // Direct, hardware-aligned memory rendering
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef contextRef = CGBitmapContextCreate(
        surface->pixels, width, height, 8, width * 4, colorSpace,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    );
    CGColorSpaceRelease(colorSpace);

    if (!contextRef) {
        SDL_FreeSurface(surface);
        CGImageRelease(imageRef);
        return nullptr;
    }

    CGContextDrawImage(contextRef, CGRectMake(0, 0, width, height), imageRef);
    CGContextRelease(contextRef);
    CGImageRelease(imageRef);

    return surface;
}
#endif

// Custom theme mapping directly to your artwork colors
void ApplyCosmicJadeTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]             = ImVec4(0.04f, 0.05f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.05f, 0.06f, 0.10f, 1.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.04f, 0.05f, 0.08f, 0.95f);
    colors[ImGuiCol_Border]               = ImVec4(0.83f, 0.71f, 0.23f, 0.40f);

    colors[ImGuiCol_Header]               = ImVec4(0.02f, 0.22f, 0.15f, 0.80f); 
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.05f, 0.35f, 0.24f, 0.85f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.09f, 0.55f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab]                  = ImVec4(0.02f, 0.22f, 0.15f, 0.60f);
    colors[ImGuiCol_TabHovered]           = ImVec4(0.05f, 0.35f, 0.24f, 0.80f);
    colors[ImGuiCol_TabActive]            = ImVec4(0.09f, 0.55f, 0.38f, 1.00f);

    colors[ImGuiCol_TitleBg]              = ImVec4(0.02f, 0.15f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.02f, 0.22f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.07f, 0.10f, 0.15f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.52f, 0.45f, 0.15f, 0.60f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.83f, 0.71f, 0.23f, 0.80f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.09f, 0.55f, 0.38f, 1.00f);

    colors[ImGuiCol_Text]                 = ImVec4(0.92f, 0.95f, 0.94f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.02f, 0.22f, 0.15f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.83f, 0.71f, 0.23f, 1.00f);

    style.WindowRounding   = 4.0f;
    style.FrameRounding    = 3.0f;
    style.WindowBorderSize = 1.0f;
}

void ScanGamesDirectory() {
    g_scanned_games.clear();
    std::string path = "../../GAMES";
    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (fs::is_directory(entry.path())) {
                    g_scanned_games.push_back(entry.path().filename().string());
                }
            }
        } else {
            fs::create_directories(path);
        }
    } catch (...) {}
}

bool CreateNewGameProject(const std::string& name) {
    if (name.empty()) return false;
    std::string root_path = "../../GAMES/" + name;
    std::string romdisk_path = root_path + "/romdisk";
    try {
        if (fs::exists(root_path)) return false;
        fs::create_directories(romdisk_path);
        ScanGamesDirectory();
        return true;
    } catch (...) {
        return false;
    }
}

void SaveTimePieceProject(const std::string& filepath, const std::vector<EditorBrush>& brushes, const std::vector<EditorEntity>& entities) {
    std::ofstream out(filepath);
    if (!out) return;
    out << "[Workspace]\n";
    out << "CameraPos " << g_cam_pos.x << " " << g_cam_pos.y << " " << g_cam_pos.z << "\n";
    out << "CameraRot " << g_cam_rot.x << " " << g_cam_rot.y << " " << g_cam_rot.z << "\n";
    out << "GridSnap " << (g_grid_snap ? 1 : 0) << "\n";
    out << "GridUnits " << g_grid_units << "\n";
    out << "AudioDeckTime " << g_audio_deck.current_time << "\n";

    out << "\n[Brushes] " << brushes.size() << "\n";
    for (const auto& b : brushes) {
        out << b.bounds.min.x << " " << b.bounds.min.y << " " << b.bounds.min.z << " "
            << b.bounds.max.x << " " << b.bounds.max.y << " " << b.bounds.max.z << " "
            << b.color << " " << b.texture << " " << b.contract << "\n";
    }

    out << "\n[Entities] " << entities.size() << "\n";
    for (const auto& e : entities) {
        out << e.classname << " "
            << e.position.x << " " << e.position.y << " " << e.position.z << " "
            << e.rotation.x << " " << e.rotation.y << " " << e.rotation.z << " "
            << e.targetname << " " << e.target << "\n";
    }
    out.close();
}

void LoadTimePieceProject(const std::string& filepath, std::vector<EditorBrush>& brushes, std::vector<EditorEntity>& entities) {
    std::ifstream in(filepath);
    if (!in) return;
    std::string line, section;
    brushes.clear();
    entities.clear();

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') {
            section = line;
            continue;
        }
        std::stringstream ss(line);
        if (section == "[Workspace]") {
            std::string key; ss >> key;
            if (key == "CameraPos")   ss >> g_cam_pos.x >> g_cam_pos.y >> g_cam_pos.z;
            if (key == "CameraRot")   ss >> g_cam_rot.x >> g_cam_rot.y >> g_cam_rot.z;
            if (key == "GridSnap")    { int val; ss >> val; g_grid_snap = (val == 1); }
            if (key == "GridUnits")   ss >> g_grid_units;
            if (key == "AudioDeckTime") ss >> g_audio_deck.current_time;
        } 
        else if (section.rfind("[Brushes]", 0) == 0) {
            EditorBrush b;
            ss >> b.bounds.min.x >> b.bounds.min.y >> b.bounds.min.z
               >> b.bounds.max.x >> b.bounds.max.y >> b.bounds.max.z
               >> b.color >> b.texture >> b.contract;
            g_brushes.push_back(b);
        } 
        else if (section.rfind("[Entities]", 0) == 0) {
            EditorEntity e;
            ss >> e.classname 
               >> e.position.x >> e.position.y >> e.position.z
               >> e.rotation.x >> e.rotation.y >> e.rotation.z
               >> e.targetname >> e.target;
            entities.push_back(e);
        }
    }
    in.close();
}

bool CompileBlenderObjToFmg(const std::string& obj_filepath, const std::string& fmg_filepath) {
    std::ifstream file(obj_filepath);
    if (!file.is_open()) return false;

    std::vector<vec3_t> temp_positions;
    struct UV { float u, v; };
    std::vector<UV> temp_uvs;
    std::vector<FrogVertex> compiled_vertices;

    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("v ", 0) == 0) {
            std::stringstream ss(line.substr(2));
            vec3_t pos = {0.0f, 0.0f, 0.0f, 0.0f};
            ss >> pos.x >> pos.y >> pos.z;
            temp_positions.push_back(pos);
        } 
        else if (line.rfind("vt ", 0) == 0) {
            std::stringstream ss(line.substr(3));
            UV uv = {0.0f, 0.0f};
            ss >> uv.u >> uv.v;
            uv.v = 1.0f - uv.v; 
            temp_uvs.push_back(uv);
        } 
        else if (line.rfind("f ", 0) == 0) {
            std::stringstream ss(line.substr(2));
            std::string v1, v2, v3;
            ss >> v1 >> v2 >> v3;

            auto ParseVertexToken = [&](const std::string& token) -> FrogVertex {
                int p_idx = 0, t_idx = 0, n_idx = 0;
                if (token.find('/') == std::string::npos) {
                    p_idx = std::stoi(token);
                } else {
                    std::stringstream ts(token);
                    std::string part;
                    if (std::getline(ts, part, '/')) p_idx = std::stoi(part);
                    if (std::getline(ts, part, '/')) {
                        if (!part.empty()) t_idx = std::stoi(part);
                    }
                    if (std::getline(ts, part, '/')) {
                        if (!part.empty()) n_idx = std::stoi(part);
                    }
                }

                FrogVertex compiled_v = {};
                if (p_idx > 0) compiled_v.x = temp_positions[p_idx - 1].x;
                if (p_idx > 0) compiled_v.y = temp_positions[p_idx - 1].y;
                if (p_idx > 0) compiled_v.z = temp_positions[p_idx - 1].z;

                if (t_idx > 0) {
                    compiled_v.u = temp_uvs[t_idx - 1].u;
                    compiled_v.v = temp_uvs[t_idx - 1].v;
                }
                compiled_v.argb = 0xFFFFFFFF;  
                compiled_v.oargb = 0x00000000; 

                return compiled_v;
            };

            compiled_vertices.push_back(ParseVertexToken(v1));
            compiled_vertices.push_back(ParseVertexToken(v2));
            compiled_vertices.push_back(ParseVertexToken(v3));
        }
    }
    file.close();

    std::ofstream out(fmg_filepath, std::ios::binary);
    if (!out) return false;
    out.write((char*)compiled_vertices.data(), compiled_vertices.size() * sizeof(FrogVertex));
    out.close();
    return true;
}

void ExportLevelBinary(const char* filepath, const std::vector<EditorBrush>& brushes, const std::vector<EditorEntity>& entities) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) return;

    uint32_t brush_cnt = brushes.size();
    uint32_t entity_cnt = entities.size();

    out.write((char*)&brush_cnt, sizeof(uint32_t));
    out.write((char*)&entity_cnt, sizeof(uint32_t));

    for (const auto& b : brushes) {
        out.write((char*)&b.bounds, sizeof(frog_aabb_t));
        out.write((char*)&b.color, sizeof(uint32_t));
        out.write((char*)b.texture, 32);
    }

    for (const auto& e : entities) {
        out.write((char*)e.classname, 32);
        out.write((char*)&e.position, sizeof(vec3_t));
        out.write((char*)&e.rotation, sizeof(vec3_t));
        out.write((char*)e.targetname, 32);
        out.write((char*)e.target, 32);
    }

    out.close();
}
