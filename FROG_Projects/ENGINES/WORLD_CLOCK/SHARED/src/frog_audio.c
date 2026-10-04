//## COPYRIGHT (C) 2026 FROGHOUSE GAMES Elisha Caez

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h> 
#include <dc/sound/stream.h> 
#include <dc/spu.h>
#include <dc/g2bus.h> 
#include <kos/fs.h>

#include "frog_audio.h"
#include "frog_util.h"
#include "frog_physics.h" 
#include "frog_air_signal.h" 
#include "world_clock.h"     

// Master sound driver initialization guard to prevent double-init conflicts
volatile bool g_sound_initted = false;

static snd_stream_hnd_t stream_hnd = SND_STREAM_INVALID;
static file_t stream_file = FILEHND_INVALID;
static uint8_t stream_buffer[SND_STREAM_BUFFER_MAX]; 
static uint32_t master_sine_addr = 0;

static float tape_speed = 1.0f;    
static float tape_target = 1.0f;   

static FrogAirCrossover master_crossover = {0.0f, 0.0f, 0.0f, 0.0f};

static uint32_t ghost_envelopes[64] = {0};
static uint32_t envelope_decays[64] = {0};

#define RING_BUFFER_SIZE (16 * 1024) // Reclaimed 16KB of precious static memory
static uint8_t ring_buffer[RING_BUFFER_SIZE];
static volatile uint32_t ring_head = 0; 
static volatile uint32_t ring_tail = 0; 

// --- [ OPTIMIZED AICA MAILBOX COMMAND BUFFER ] ---
#define MAX_MAILBOX_CMDS 16
typedef struct {
    uint32_t cmd;
    uint32_t ch;
    uint32_t val1;
    uint32_t val2;
} __attribute__((aligned(32))) AicaMailboxCmd;

static AicaMailboxCmd mailbox_queue[MAX_MAILBOX_CMDS] __attribute__((aligned(32)));
static volatile uint32_t mailbox_count = 0;

// Command codes for granular loop window and hardware effects steering
#define CMD_SET_START_ADDR 0x05
#define CMD_SET_LOOP_WINDOW 0x06
#define CMD_SET_FORMAT      0x07 
#define CMD_SET_EFFECT_SEND 0x08 
#define CMD_SET_DSP_PROGRAM 0x09 
#define CMD_SET_WAVETABLE_SYNTH 0x0a

// SPU-SRAM Telemetry Exchange Block (located safely at the upper boundary of physical SRAM)
#define SPU_TELEMETRY_ADDR 0xa09f0000

typedef struct {
    uint32_t magic; // 0x46524f47 - "FROG"
    float sub_bass;
    float mids;
    float treble;
    float mid_energy;
    float side_energy;
    float pan_bias;
} __attribute__((packed)) SpuTelemetry;

extern WorldClock wc; 

static void frog_aica_direct_start(int ch, uint32_t addr, uint32_t freq, int loop) {
    uint32_t packet[4] = { 0x04, ch, freq, addr | (loop ? 0x80000000 : 0) };
    snd_sh4_to_aica(packet, 4);
}

void frog_audio_init_ghost_channels() {
    int16_t sine_table[1024];
    for(int i = 0; i < 1024; i++) {
        sine_table[i] = (int16_t)(sinf((float)i / 1024.0f * 6.28318f) * 32767.0f);
    }
    master_sine_addr = snd_mem_malloc(2048);
    if(!master_sine_addr) return;
    
    spu_memload(master_sine_addr, sine_table, 2048);
    
    frog_aica_direct_start(GHOST_CH_WAVE_ALPHA, master_sine_addr, 4410,  1);
    frog_aica_direct_start(GHOST_CH_WAVE_BETA,  master_sine_addr, 11025, 1);
    frog_aica_direct_start(GHOST_CH_VIBRATION, master_sine_addr, 44100, 1);
}

void frog_audio_effects_tape_stop() { tape_target = 0.0f; }
void frog_audio_effects_tape_start() { tape_target = 1.0f; wc.audio.is_paused = false; }
void frog_audio_effects_scratch(float amount) { tape_speed = 1.0f + (amount * 2.0f); }
void frog_audio_stream_pause() { wc.audio.is_paused = !wc.audio.is_paused; }

void frog_audio_set_volume(int vol) {
    if (stream_hnd != SND_STREAM_INVALID) snd_stream_volume(stream_hnd, vol);
}

void frog_audio_set_pitch(uint32_t channel, float pitch_multiplier) {
    uint32_t freq = (uint32_t)(44100.0f * pitch_multiplier);
    uint32_t packet[4] = { 0x01, channel, freq, 0 }; 
    snd_sh4_to_aica(packet, 4);
}

void frog_audio_queue_cmd(uint32_t cmd, uint32_t ch, uint32_t val1, uint32_t val2) {
    uint32_t count = mailbox_count;
    if (count < MAX_MAILBOX_CMDS) {
        mailbox_queue[count].cmd = cmd;
        mailbox_queue[count].ch = ch;
        mailbox_queue[count].val1 = val1;
        mailbox_queue[count].val2 = val2;
        mailbox_count = count + 1;
    }
}

// Optimized flusher executing a single G2 FIFO write block at the end of execution
void frog_audio_mailbox_flush() {
    uint32_t count = mailbox_count;
    if (count == 0) return;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t packet[4] = {
            mailbox_queue[i].cmd,
            mailbox_queue[i].ch,
            mailbox_queue[i].val1,
            mailbox_queue[i].val2
        };
        snd_sh4_to_aica(packet, 4);
    }
    g2_fifo_wait(); // Call once at the end of the batch operation to prevent stalls
    mailbox_count = 0;
}

static void* frog_internal_stream_cb(snd_stream_hnd_t hnd, int smp_req, int *smp_recv) {
    (void)hnd;
    if (wc.audio.is_paused || tape_speed < 0.01f) {
        *smp_recv = 0; return NULL;
    }

    if (tape_speed != tape_target) {
        tape_speed += (tape_target - tape_speed) * 0.05f; 
        frog_audio_set_pitch(0, tape_speed); 
        frog_audio_set_pitch(1, tape_speed);
    }

    uint32_t bytes_req = smp_req * 4;
    
    uint32_t head = ring_head;
    uint32_t tail = ring_tail;
    uint32_t fill = (head >= tail) ? (head - tail) : (RING_BUFFER_SIZE - tail + head);

    if (fill < bytes_req) {
        if (wc.audio.is_done) {
            *smp_recv = 0;
            return NULL;
        }
        memset(stream_buffer, 0, bytes_req);
        *smp_recv = smp_req;
        return stream_buffer;
    }

    uint32_t space_to_end = RING_BUFFER_SIZE - tail;
    if (bytes_req > space_to_end) {
        memcpy(stream_buffer, &ring_buffer[tail], space_to_end);
        memcpy(&stream_buffer[space_to_end], &ring_buffer[0], bytes_req - space_to_end);
        ring_tail = bytes_req - space_to_end;
    } else {
        memcpy(stream_buffer, &ring_buffer[tail], bytes_req);
        ring_tail = (tail + bytes_req) % RING_BUFFER_SIZE;
    }

    // --- [ 100% ARM7 SPU SOUND TELEMETRY BYPASS ] ---
    // Read cached values directly from SRAM if our custom firmware is loaded,
    // completely eliminating software DSP math overhead on the SH-4.
    volatile SpuTelemetry *telemetry = (volatile SpuTelemetry *)SPU_TELEMETRY_ADDR;
    
    if (telemetry->magic == 0x46524f47) { // Magic code: "FROG"
        wc.sub_bass    = telemetry->sub_bass;
        wc.mids        = telemetry->mids;
        wc.treble      = telemetry->treble;
        wc.mid_energy  = telemetry->mid_energy;
        wc.side_energy = telemetry->side_energy;
        wc.pan_bias    = telemetry->pan_bias;
    } else {
        // Fallback: Calculate math on SH-4 if SPU-Offloading code is not running
        int num_samples = bytes_req / 2;
        int16_t *pcm = (int16_t *)stream_buffer;
        float peak_l = 0, peak_m = 0, peak_h = 0;
        
        float left_accum = 0.0f;
        float right_accum = 0.0f;
        
        float peak_mid = 0.0f;
        float peak_side = 0.0f;

        for (int i = 0; i < num_samples; i += 2) {
            float left_sample = (float)pcm[i] / 32768.0f;
            float right_sample = (float)pcm[i+1] / 32768.0f;
            
            left_accum += fabsf(left_sample);
            right_accum += fabsf(right_sample);

            float mid_s = (left_sample + right_sample) * 0.5f;  
            float side_s = (left_sample - right_sample) * 0.5f; 
            
            if (fabsf(mid_s) > peak_mid)   peak_mid = fabsf(mid_s);
            if (fabsf(side_s) > peak_side) peak_side = fabsf(side_s);

            float s, m, h;
            frog_air_isolate_bands(&master_crossover, mid_s, &s, &m, &h);
            if (fabsf(s) > peak_l) peak_l = fabsf(s);
            if (fabsf(m) > peak_m) peak_m = fabsf(m);
            if (fabsf(h) > peak_h) peak_h = fabsf(h);
        }
        
        wc.sub_bass = (wc.sub_bass * 0.8f) + (peak_l * 0.2f);
        wc.mids     = (wc.mids     * 0.8f) + (peak_m * 0.2f);
        wc.treble   = (wc.treble   * 0.8f) + (peak_h * 0.2f);
        
        wc.mid_energy = (wc.mid_energy * 0.8f) + (peak_mid * 0.2f);
        wc.side_energy = (wc.side_energy * 0.8f) + (peak_side * 0.2f);

        float total_energy = left_accum + right_accum;
        if (total_energy > 0.001f) {
            float raw_pan = (right_accum - left_accum) / total_energy; 
            wc.pan_bias = (wc.pan_bias * 0.8f) + (raw_pan * 0.2f);
        }
    }

    *smp_recv = smp_req;
    return stream_buffer;
}

void frog_audio_stream_start(const char *filepath) {
    if(stream_file != FILEHND_INVALID) fs_close(stream_file);
    stream_file = fs_open(filepath, O_RDONLY);
    if(stream_file == FILEHND_INVALID) return;
    
    ring_head = 0;
    ring_tail = 0;
    memset(ring_buffer, 0, RING_BUFFER_SIZE);

    int read = fs_read(stream_file, ring_buffer, 16384);
    if (read > 0) {
        ring_head = read;
    }

    if(stream_hnd == SND_STREAM_INVALID) 
        stream_hnd = snd_stream_alloc(frog_internal_stream_cb, SND_STREAM_BUFFER_MAX);
    snd_stream_start(stream_hnd, 44100, 1); 
    tape_speed = 1.0f; tape_target = 1.0f;
    wc.audio.is_done = false;
}

void frog_audio_stream_stop() {
    if (stream_hnd != SND_STREAM_INVALID) {
        snd_stream_stop(stream_hnd);
    }
    if (stream_file != FILEHND_INVALID) {
        fs_close(stream_file);
        stream_file = FILEHND_INVALID;
    }
    wc.audio.is_done = true;
}

void frog_audio_stream_update() {
    if (stream_file == FILEHND_INVALID || wc.audio.is_paused) return;

    uint32_t head = ring_head;
    uint32_t tail = ring_tail;
    uint32_t fill = (head >= tail) ? (head - tail) : (RING_BUFFER_SIZE - tail + head);
    uint32_t free_space = RING_BUFFER_SIZE - fill - 1;

    if (free_space >= 8192 && !wc.audio.is_done) {
        uint32_t bytes_to_read = 8192;
        
        uint32_t space_to_end = RING_BUFFER_SIZE - head;
        if (bytes_to_read > space_to_end) {
            int read1 = fs_read(stream_file, &ring_buffer[head], space_to_end);
            if (read1 <= 0) { wc.audio.is_done = true; return; }
            
            int read2 = fs_read(stream_file, &ring_buffer[0], bytes_to_read - space_to_end);
            if (read2 < 0) read2 = 0;
            if (read2 == 0) wc.audio.is_done = true;
            
            ring_head = read2;
        } else {
            int read = fs_read(stream_file, &ring_buffer[head], bytes_to_read);
            if (read <= 0) { wc.audio.is_done = true; return; }
            ring_head = (head + read) % RING_BUFFER_SIZE;
        }
    }

    if (stream_hnd != SND_STREAM_INVALID) {
        snd_stream_poll(stream_hnd);
    }
}

uint32_t frog_audio_get_position(uint32_t channel) { 
    g2_fifo_wait();
    return (uint32_t)snd_get_pos(channel); 
}

void frog_audio_init() {
    if (g_sound_initted) {
        printf("FROG_AUDIO: Sound already initialized by host or kernel. Bypassing snd_init.\n");
        return;
    }

    if (snd_init() < 0) {
        frog_util_log("AUDIO ERROR: SPU hardware initialization failed.");
        return;
    }
    if (snd_stream_init() < 0) return;
    
    g_sound_initted = true;
    
    frog_air_crossover_init(&master_crossover, 0.05f, 0.40f);
    
    frog_audio_init_ghost_channels();
    printf("FROG_AUDIO: 'Mano de Oro' Deck Patched & Ready.\n");
}

void frog_audio_stream_rewind() {
    if (stream_file == FILEHND_INVALID) return;
    fs_seek(stream_file, 0, SEEK_SET);
    
    ring_head = 0;
    ring_tail = 0;
    memset(ring_buffer, 0, RING_BUFFER_SIZE);
    
    int read = fs_read(stream_file, ring_buffer, 16384);
    if (read > 0) {
        ring_head = read;
    }

    wc.audio.is_done = false;
    wc.audio.is_paused = false;
    if (stream_hnd != SND_STREAM_INVALID) {
        snd_stream_start(stream_hnd, 44100, 1);
    }
    frog_util_log("AUDIO: Deck Rewound to Start.");
}

void frog_audio_trigger_momentum(uint32_t channel, uint32_t decay_rate) {
    if (channel < 64) {
        ghost_envelopes[channel] = 255; 
        envelope_decays[channel] = decay_rate;
        frog_audio_set_pitch(channel, 1.0f);
    }
}

uint32_t frog_audio_get_envelope_vol(uint32_t channel) {
    if (channel < 64) {
        uint32_t decay = envelope_decays[channel] ? envelope_decays[channel] : 2;
        if (ghost_envelopes[channel] > decay) {
            ghost_envelopes[channel] -= decay;
        } else {
            ghost_envelopes[channel] = 0;
        }
        return ghost_envelopes[channel];
    }
    return 0;
}

void frog_audio_set_crossovers(float low_coeff, float high_coeff) {
    master_crossover.lp_coeff = low_coeff;
    master_crossover.hp_coeff = high_coeff;
}

void frog_audio_set_pan(uint32_t channel, int pan) {
    uint32_t packet[4] = { 0x02, channel, 255, pan }; 
    snd_sh4_to_aica(packet, 4);
}

void frog_audio_play_spatial_sfx(uint32_t channel, vec3_t sfx_pos, vec3_t cam_pos, vec3_t cam_target, vec3_t cam_up, float max_radius) {
    vec3_t diff = vec3_sub(sfx_pos, cam_pos);
    float dist = vec3_length(diff);
    
    float vol_scale = 1.0f - (dist / max_radius);
    if (vol_scale < 0.0f) vol_scale = 0.0f;
    int vol = (int)(vol_scale * 255.0f);

    vec3_t cam_dir = vec3_normalize(vec3_sub(cam_target, cam_pos));
    vec3_t cam_right = vec3_normalize(vec3_cross(cam_dir, cam_up));
    
    float pan_val = vec3_dot(vec3_normalize(diff), cam_right);
    int pan = (int)((pan_val + 1.0f) * 127.0f); 

    uint32_t packet[4] = { 0x02, channel, vol, pan }; 
    snd_sh4_to_aica(packet, 4);
}

void frog_audio_set_granular_loop(uint32_t channel, uint32_t start_addr, uint32_t loop_start, uint32_t loop_end) {
    frog_audio_queue_cmd(CMD_SET_START_ADDR, channel, start_addr, 0);
    frog_audio_queue_cmd(CMD_SET_LOOP_WINDOW, channel, loop_start, loop_end);
}

void frog_audio_set_format(uint32_t channel, uint32_t format) {
    frog_audio_queue_cmd(CMD_SET_FORMAT, channel, format, 0);
}

void frog_audio_set_effect_send(uint32_t channel, uint32_t dsp_level) {
    frog_audio_queue_cmd(CMD_SET_EFFECT_SEND, channel, dsp_level, 0);
}

void frog_audio_apply_environmental_acoustics(int active_channels_mask, FrogEnvZone zone) {
    uint32_t dsp_program_id = 0;
    uint32_t send_level = 0;

    switch (zone) {
        case ENV_DRY:
            dsp_program_id = 0; 
            send_level = 0;
            break;
        case ENV_CAVE:
            dsp_program_id = 1; 
            send_level = 12;    
            break;
        case ENV_WAREHOUSE:
            dsp_program_id = 2; 
            send_level = 8;     
            break;
        case ENV_UNDERWATER:
            dsp_program_id = 3; 
            send_level = 15;    
            break;
    }

    frog_audio_queue_cmd(CMD_SET_DSP_PROGRAM, 0, dsp_program_id, 0);

    for (int ch = 0; ch < 64; ch++) {
        if ((active_channels_mask & (1ULL << ch)) != 0) {
            frog_audio_set_effect_send(ch, send_level);
        }
    }
}

void frog_audio_set_wavetable_synth(uint32_t channel, uint32_t wave_type, float frequency, float modulation_depth) {
    uint32_t freq_val = (uint32_t)(frequency * 1000.0f);
    uint32_t mod_val = (uint32_t)(modulation_depth * 1000.0f);
    uint32_t packed_args = (freq_val << 16) | (mod_val & 0xFFFF);
    
    frog_audio_queue_cmd(CMD_SET_WAVETABLE_SYNTH, channel, wave_type, packed_args);
}
