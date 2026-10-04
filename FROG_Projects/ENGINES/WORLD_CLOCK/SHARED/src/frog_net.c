#include "frog_net.h"
#include "frog_util.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int frog_net_init_session(FrogNetSession *session, const char *ip_addr, int port) {
    if (!session) return -1;

    session->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (session->socket_fd < 0) {
        frog_util_log("NET ERROR: Could not allocate UDP socket.");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip_addr);

    int flags = 1;
    setsockopt(session->socket_fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    
    memset(session->players, 0, sizeof(session->players));
    frog_util_log("NET: MMO Broadband / Modem Socket Online.");
    return 0;
}

void frog_net_save_state(FrogNetSession *session, int client_idx, vec3_t pos, vec3_t vel, vec3_t acc, FrogNetInput input, WorldClock *wc) {
    if (!session || !wc || client_idx < 0 || client_idx >= NET_MAX_CLIENTS) return;

    FrogNetPlayerRegistry *player = &session->players[client_idx];
    player->active = true;

    uint32_t idx = player->slice_cursor;
    player->slices[idx].position = pos;
    player->slices[idx].velocity = vel;
    player->slices[idx].acceleration = acc; // Track acceleration for dynamic prediction
    player->slices[idx].spu_sample_id = wc->audio.sample_count;
    player->slices[idx].last_input = input;

    player->slice_cursor = (idx + 1) % NET_ROLLBACK_DEPTH;
}

void frog_net_rollback_state(FrogNetSession *session, int client_idx, uint32_t target_sample_id, 
                              vec3_t *out_pos, vec3_t *out_vel) {
    if (!session || !out_pos || !out_vel || client_idx < 0 || client_idx >= NET_MAX_CLIENTS) return;

    FrogNetPlayerRegistry *player = &session->players[client_idx];
    if (!player->active) return;

    for (int i = 0; i < NET_ROLLBACK_DEPTH; i++) {
        if (player->slices[i].spu_sample_id == target_sample_id) {
            *out_pos = player->slices[i].position;
            *out_vel = player->slices[i].velocity;
            frog_util_log("NETPLAY: Multi-Client AICA-Locked Rollback Executed.");
            return;
        }
    }
    frog_util_log("NETPLAY WARNING: Client target SPU sample depth passed.");
}

// [JOINT 6: DETERMINISTIC DEAD-RECKONING PREDICTOR]
// Extrapolates physics pathways if a network packet is late.
void frog_net_predict_state(FrogNetSession *session, int client_idx, uint32_t elapsed_samples, 
                             vec3_t *out_pos, vec3_t *out_vel) {
    if (!session || !out_pos || !out_vel || client_idx < 0 || client_idx >= NET_MAX_CLIENTS) return;

    FrogNetPlayerRegistry *player = &session->players[client_idx];
    if (!player->active) return;

    // Grab the most recent historical slice
    uint32_t last_idx = (player->slice_cursor == 0) ? (NET_ROLLBACK_DEPTH - 1) : (player->slice_cursor - 1);
    FrogStateSlice *last_slice = &player->slices[last_idx];

    float dt = (float)elapsed_samples / 44100.0f;

    // Standard kinematic equations:
    // s = s_0 + v * t + 0.5 * a * t^2
    vec3_t term_v = vec3_scale(last_slice->velocity, dt);
    vec3_t term_a = vec3_scale(last_slice->acceleration, 0.5f * dt * dt);
    *out_pos = vec3_add(last_slice->position, vec3_add(term_v, term_a));

    // v = v_0 + a * t
    *out_vel = vec3_add(last_slice->velocity, vec3_scale(last_slice->acceleration, dt));
}
