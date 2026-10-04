#include "frog_portal.h"
#include <dc/matrix.h>

// Fast, low-overhead projection helper to check if a portal is inside the view frustum
static bool is_portal_in_frustum(const FrogPortal *portal) {
    // Project the 4 corners of the portal quad using the active XMTRX register
    for (int i = 0; i < 4; i++) {
        float x = portal->vertices[i].x;
        float y = portal->vertices[i].y;
        float z = portal->vertices[i].z;

        mat_trans_single(x, y, z);

        // If at least one vertex projects onto the screen bounds, drawing is permitted
        if (z > 0.1f && x >= -z && x <= z && y >= -z && y <= z) {
            return true;
        }
    }
    return false;
}

void frog_portal_cull_and_draw(FrogRoomGraph *graph, vec3_t cam_pos, matrix_t *frustum_matrix) {
    if (!graph || graph->room_count == 0) return;

    // Retrieve active room pointer
    FrogRoom *current_room = NULL;
    for (uint32_t i = 0; i < graph->room_count; i++) {
        if (graph->rooms[i].id == graph->active_room_id) {
            current_room = &graph->rooms[i];
            break;
        }
    }

    if (!current_room) return;

    // Load active transformation frustum matrix to the SH-4 XMTRX
    mat_load(frustum_matrix);

    // Render all meshes belonging to the player's active room
    for (uint32_t m = 0; m < current_room->mesh_count; m++) {
        if (current_room->meshes[m]) {
            frog_mesh_render(current_room->meshes[m], true);
        }
    }

    // Check visibility portals to draw adjacent rooms
    for (uint32_t p = 0; p < current_room->portal_count; p++) {
        FrogPortal *portal = &current_room->portals[p];

        if (portal->is_open && portal->target_room) {
            // Test if portal plane boundaries reside within frustum
            if (is_portal_in_frustum(portal)) {
                
                // Recursively render connected target room meshes
                FrogRoom *target = portal->target_room;
                for (uint32_t tm = 0; tm < target->mesh_count; tm++) {
                    if (target->meshes[tm]) {
                        frog_mesh_render(target->meshes[tm], true);
                    }
                }
            }
        }
    }
}
