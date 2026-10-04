#ifndef FROG_PORTAL_H
#define FROG_PORTAL_H

#include <stdbool.h>
#include "frog_mesh.h"
#include "frog_math_3d.h"

#define MAX_PORTALS_PER_ROOM 4
#define MAX_ROOM_MESHES      8

struct FrogRoom;

typedef struct {
    vec3_t          vertices[4]; // 2D Quad plane in 3D space acting as the portal window
    struct FrogRoom *target_room; // Pointer to the connected room node
    bool            is_open;     // If false, the door is closed and culls immediately
} FrogPortal;

typedef struct FrogRoom {
    uint32_t    id;
    FrogMesh    *meshes[MAX_ROOM_MESHES]; // Geometry blocks assigned to this room
    uint32_t    mesh_count;
    FrogPortal  portals[MAX_PORTALS_PER_ROOM];
    uint32_t    portal_count;
} FrogRoom;

typedef struct {
    FrogRoom *rooms;
    uint32_t room_count;
    uint32_t active_room_id;
} FrogRoomGraph;

/**
 * [PORTAL SECTOR CULLER]
 * Recursively walks the portal graph starting from the active room, 
 * adding visible room geometry to the rendering pipeline while culling hidden rooms.
 */
void frog_portal_cull_and_draw(FrogRoomGraph *graph, vec3_t cam_pos, matrix_t *frustum_matrix);

#endif
