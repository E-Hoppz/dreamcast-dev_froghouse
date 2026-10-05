#ifndef FROG_NET_H
#define FROG_NET_H

#include <stdint.h>
#include <stdbool.h>
#include "world_clock.h"
#include "frog_math_3d.h"

#define NET_ROLLBACK_DEPTH 16 
#define NET_MAX_CLIENTS    8  

typedef struct {
    uint32_t buttons;
    float    stick_x;
    float    stick_y;
} FrogNetInput;

typedef struct {
    uint32_t     spu_sample_id; 
    FrogNetInput inputs[NET_MAX_CLIENTS]; 
} FrogNetPacket;

typedef struct {
    vec3_t       position;
    vec3_t       velocity;
    vec3_t       acceleration; // [NEW] Track acceleration vectors for Dead-Reckoning
    uint32_t     spu_sample_id;
    FrogNetInput last_input;
} __attribute__((aligned(32))) FrogStateSlice;

typedef struct {
    FrogStateSlice slices[NET_ROLLBACK_DEPTH];
    uint32_t       slice_cursor;
    bool           active;
} FrogNetPlayerRegistry;

typedef struct {
    FrogNetPlayerRegistry players[NET_MAX_CLIENTS]; 
    int                   socket_fd;
} FrogNetSession;

int  frog_net_init_session(FrogNetSession *session, const char *ip_addr, int port);
void frog_net_save_state(FrogNetSession *session, int client_idx, vec3_t pos, vec3_t vel, vec3_t acc, FrogNetInput input, WorldClock *wc);
void frog_net_rollback_state(FrogNetSession *session, int client_idx, uint32_t target_sample_id, 
                              vec3_t *out_pos, vec3_t *out_vel);

// [NEW] Deterministic Dead-Reckoning Predictor
void frog_net_predict_state(FrogNetSession *session, int client_idx, uint32_t elapsed_samples, 
                             vec3_t *out_pos, vec3_t *out_vel);

#endif
