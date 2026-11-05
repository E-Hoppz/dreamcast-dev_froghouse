#ifndef FROG_AUDIO_H
#define FROG_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "frog_math_3d.h"
#include <dc/sound/sound.h> 
#include <dc/sound/sfxmgr.h> 

#ifdef __cplusplus
extern "C" {
#endif

#define GHOST_CH_WAVE_ALPHA   60  
#define GHOST_CH_WAVE_BETA    61  
#define GHOST_CH_VIBRATION    62  
#define GHOST_CH_RHYTHM_SYNC  63  

#define AICA_SM_16BIT         0x00 
#define AICA_SM_8BIT          0x01 
#define AICA_SM_ADPCM         0x02 

typedef enum {
    ENV_DRY,         
    ENV_CAVE,        
    ENV_WAREHOUSE,   
    ENV_UNDERWATER   
} FrogEnvZone;

typedef struct AudioClock {
    uint32_t sample_count; 
    uint64_t monotonic_samples;   
    uint32_t sample_rate;     
    uint32_t current_offset;  
    float    pitch_scale;     
    bool     is_paused;       
    bool     is_done;
} AudioClock;

extern volatile bool g_sound_initted;
struct frog_cam_t;

uint64_t frog_audio_get_monotonic_samples(void);

void frog_audio_init(void);               
void frog_audio_init_ghost_channels(void); 

void frog_audio_stream_start(const char *filepath);
void frog_audio_stream_stop(void);
void frog_audio_stream_pause(void);
void frog_audio_stream_rewind(void);
void frog_audio_stream_update(void);

void frog_audio_effects_tape_stop(void);  
void frog_audio_effects_tape_start(void); 
void frog_audio_effects_scratch(float amount); 

void frog_audio_set_pitch(uint32_t channel, float pitch_multiplier);
void frog_audio_trigger_momentum(uint32_t channel, uint32_t decay_rate);
uint32_t frog_audio_get_envelope_vol(uint32_t channel);
uint32_t frog_audio_get_position(uint32_t channel);
void frog_audio_set_volume(int vol); 
void frog_audio_set_crossovers(float low_coeff, float high_coeff);
void frog_audio_set_pan(uint32_t channel, int pan);

void frog_audio_play_spatial_sfx(uint32_t channel, vec3_t sfx_pos, vec3_t cam_pos, vec3_t cam_target, vec3_t cam_up, float max_radius);
void frog_audio_queue_cmd(uint32_t cmd, uint32_t ch, uint32_t val1, uint32_t val2);
void frog_audio_mailbox_flush(void);

void frog_audio_set_granular_loop(uint32_t channel, uint32_t start_addr, uint32_t loop_start, uint32_t loop_end);
void frog_audio_set_format(uint32_t channel, uint32_t format);
void frog_audio_set_effect_send(uint32_t channel, uint32_t dsp_level);
void frog_audio_apply_environmental_acoustics(uint64_t active_channels_mask, FrogEnvZone zone);
void frog_audio_set_wavetable_synth(uint32_t channel, uint32_t wave_type, float frequency, float modulation_depth);

#ifdef __cplusplus
}
#endif

#endif
