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

#define RING_BUFFER_SIZE (64 * 1024)
static uint8_t ring_buffer[RING_BUFFER_SIZE];
static volatile uint32_t ring_head = 0; 
static volatile uint32_t ring_tail = 0; 

static volatile uint64_t g_stream_sample_counter = 0;

uint64_t frog_audio_get_monotonic_samples(void) {
    return g_stream_sample_counter;
}

#define SPU_TELEMETRY_ADDR 0xa09f0000

typedef struct {
    uint32_t magic;
    float sub_bass;
    float mids;
    float treble;
    float mid_energy;
    float side_energy;
    float pan_bias;
} __attribute__((packed)) SpuTelemetry;

extern WorldClock wc; 

void frog_audio_init_ghost_channels(void) {
    int16_t sine_table[1024];
    for(int i = 0; i < 1024; i++) {
        sine_table[i] = (int16_t)(sinf((float)i / 1024.0f * 6.28318f) * 32767.0f);
    }
    master_sine_addr = snd_mem_malloc(2048);
    if(!master_sine_addr) return;
    
    spu_memload(master_sine_addr, sine_table, 2048);
}

void frog_audio_effects_tape_stop(void) { tape_target = 0.0f; }
void frog_audio_effects_tape_start(void) { tape_target = 1.0f; wc.audio.is_paused = false; }
void frog_audio_effects_scratch(float amount) { tape_speed = 1.0f + (amount * 2.0f); }
void frog_audio_stream_pause(void) { wc.audio.is_paused = !wc.audio.is_paused; }

void frog_audio_set_volume(int vol) {
    if (stream_hnd != SND_STREAM_INVALID) snd_stream_volume(stream_hnd, vol);
}

void frog_audio_set_pitch(uint32_t channel, float pitch_multiplier) {
    (void)channel;
    (void)pitch_multiplier;
}

static void* frog_internal_stream_cb(snd_stream_hnd_t hnd, int smp_req, int *smp_recv) {
    (void)hnd;
    uint32_t bytes_req = smp_req * 4;

    g_stream_sample_counter += smp_req;
    *smp_recv = smp_req;

    if (stream_file == FILEHND_INVALID || wc.audio.is_paused || wc.audio.is_done) {
        memset(stream_buffer, 0, bytes_req);
        return stream_buffer;
    }

    uint32_t head = ring_head;
    uint32_t tail = ring_tail;
    uint32_t fill = (head >= tail) ? (head - tail) : (RING_BUFFER_SIZE - tail + head);

    if (fill < bytes_req) {
        memset(stream_buffer, 0, bytes_req);
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

    volatile SpuTelemetry *telemetry = (volatile SpuTelemetry *)SPU_TELEMETRY_ADDR;
    
    if (telemetry->magic == 0x46524f47) {
        wc.sub_bass    = telemetry->sub_bass;
        wc.mids        = telemetry->mids;
        wc.treble      = telemetry->treble;
        wc.mid_energy  = telemetry->mid_energy;
        wc.side_energy = telemetry->side_energy;
        wc.pan_bias    = telemetry->pan_bias;
    } else {
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

    return stream_buffer;
}

void frog_audio_stream_start(const char *filepath) {
    if(stream_file != FILEHND_INVALID) fs_close(stream_file);
    stream_file = fs_open(filepath, O_RDONLY);
    if(stream_file == FILEHND_INVALID) return;
    
    ring_head = 0;
    ring_tail = 0;
    memset(ring_buffer, 0, RING_BUFFER_SIZE);

    int read = fs_read(stream_file, ring_buffer, 32768);
    if (read > 0) {
        ring_head = read % RING_BUFFER_SIZE;
    }

    tape_speed = 1.0f; 
    tape_target = 1.0f;
    wc.audio.is_done = false;
}

void frog_audio_stream_stop(void) {
    if (stream_file != FILEHND_INVALID) {
        fs_close(stream_file);
        stream_file = FILEHND_INVALID;
    }
    wc.audio.is_done = true;
}

void frog_audio_stream_update(void) {
    if (stream_file == FILEHND_INVALID || wc.audio.is_paused) return;

    uint32_t head = ring_head;
    uint32_t tail = ring_tail;
    uint32_t fill = (head >= tail) ? (head - tail) : (RING_BUFFER_SIZE - tail + head);
    uint32_t free_space = RING_BUFFER_SIZE - fill - 1;

    if (free_space >= 16384 && !wc.audio.is_done) {
        uint32_t bytes_to_read = 16384;
        
        uint32_t space_to_end = RING_BUFFER_SIZE - head;
        if (bytes_to_read > space_to_end) {
            int read1 = fs_read(stream_file, &ring_buffer[head], space_to_end);
            if (read1 <= 0) { wc.audio.is_done = true; return; }
            
            int read2 = fs_read(stream_file, &ring_buffer[0], bytes_to_read - space_to_end);
            if (read2 < 0) read2 = 0;
            if (read2 == 0) wc.audio.is_done = true;
            
            ring_head = read2 % RING_BUFFER_SIZE;
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

void frog_audio_init(void) {
    if (g_sound_initted) return;

    if (snd_init() < 0) {
        frog_util_log("AUDIO ERROR: SPU hardware initialization failed.");
        return;
    }
    if (snd_stream_init() < 0) return;
    
    g_sound_initted = true;
    
    frog_air_crossover_init(&master_crossover, 0.05f, 0.40f);
    frog_audio_init_ghost_channels();

    stream_hnd = snd_stream_alloc(frog_internal_stream_cb, SND_STREAM_BUFFER_MAX);
    snd_stream_start(stream_hnd, 44100, 1); 

    printf("FROG_AUDIO: 'World Clock' Hardware Ghost Flywheel Spinning at 44.1kHz.\n");
}

void frog_audio_stream_rewind(void) {
    if (stream_file == FILEHND_INVALID) return;
    fs_seek(stream_file, 0, SEEK_SET);
    
    ring_head = 0;
    ring_tail = 0;
    memset(ring_buffer, 0, RING_BUFFER_SIZE);
    
    int read = fs_read(stream_file, ring_buffer, 32768);
    if (read > 0) {
        ring_head = read % RING_BUFFER_SIZE;
    }

    wc.audio.is_done = false;
    wc.audio.is_paused = false;
    frog_util_log("AUDIO: Deck Rewound to Start.");
}

void frog_audio_trigger_momentum(uint32_t channel, uint32_t decay_rate) {
    if (channel < 64) {
        ghost_envelopes[channel] = 255; 
        envelope_decays[channel] = decay_rate;
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
    (void)channel;
    (void)pan;
}

void frog_audio_apply_environmental_acoustics(uint64_t active_channels_mask, FrogEnvZone zone) {
    (void)active_channels_mask;
    (void)zone;
}
