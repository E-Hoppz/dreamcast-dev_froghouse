#include "frog_scene.h"
#include <dc/matrix.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/**
 * Initializes a scene node to a baseline identity transform.
 */
void frog_node_init(frog_node_t *node) {
    if (!node) return;
    
    // Clear and load identity values into the local and world registers
    mat_identity();
    mat_store(&node->local_matrix);
    mat_store(&node->world_matrix);
    
    node->parent = NULL;
    node->child_count = 0;
    node->draw_func = NULL;
    
    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->children[i] = NULL;
    }
}

/**
 * Attaches a child node to a parent node's transformation chain.
 */
void frog_node_add_child(frog_node_t *parent, frog_node_t *child) {
    if (!parent || !child) return;
    
    if (parent->child_count < MAX_CHILDREN) {
        parent->children[parent->child_count++] = child;
        child->parent = parent;
    }
}

/**
 * Recursively updates the coordinate hierarchy down the tree, multiplying 
 * parent world matrices into local node transforms.
 */
void frog_node_update(frog_node_t *node, matrix_t *parent_world) {
    if (!node) return;

    // Load local matrix to SH-4 registers, apply parent world transform, and cache output
    mat_load(&node->local_matrix);
    if (parent_world) {
        mat_apply(parent_world);
    }
    mat_store(&node->world_matrix);

    // Recursively cascade calculations down all child branches
    for (int i = 0; i < node->child_count; i++) {
        frog_node_update(node->children[i], &node->world_matrix);
    }
}

/**
 * [NEW: Transform-Linked Scene Graph Hierarchical Renderer]
 * Automatically loads the node's calculated world coordinates into the SH-4 
 * hardware registers before invoking its standard rendering callbacks.
 * 
 * Tracks the target filter setting across all sub-branches recursively.
 */
void frog_node_draw(frog_node_t *node, bool filter) {
    if (!node) return;

    // Load the final calculated world coordinate space into the hardware 
    // XMTRX registers immediately prior to executing the mesh drawing callback
    if (node->draw_func) {
        mat_load(&node->world_matrix);
        node->draw_func(node);
    }

    // Recursively draw all nested children in the hierarchy, passing down the filter flag
    for (int i = 0; i < node->child_count; i++) {
        frog_node_draw(node->children[i], filter);
    }
}
