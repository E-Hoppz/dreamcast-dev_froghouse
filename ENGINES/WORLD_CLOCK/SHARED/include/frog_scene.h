#ifndef FROG_SCENE_H
#define FROG_SCENE_H

#include <kos.h>
#include "frog_math_3d.h"
#include "frog_mesh.h"

#define MAX_CHILDREN 8

typedef struct frog_node {
    matrix_t local_matrix;
    matrix_t world_matrix;
    struct frog_node *parent;
    struct frog_node *children[MAX_CHILDREN];
    int child_count;
    
    FrogMesh *mesh; // Link mesh structures directly to node
    
    void (*draw_func)(struct frog_node *self);
} frog_node_t;

void frog_node_init(frog_node_t *node);
void frog_node_add_child(frog_node_t *parent, frog_node_t *child);
void frog_node_update(frog_node_t *node, matrix_t *parent_world);
void frog_node_draw(frog_node_t *node, bool filter);

#endif
