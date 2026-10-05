#ifndef TIMEPIECE_CLOCK_H
#define TIMEPIECE_CLOCK_H

#include <SDL.h>

extern "C" {
    #include "world_clock.h"
}

// Host-only simulated SPU audio deck structure
struct AudioDeck {
    Uint8* wav_buffer;
    Uint32 wav_length;
    SDL_AudioDeviceID device_id;
    bool is_playing;
    float current_time;
    float duration;
};

// Global instance reference
extern struct AudioDeck g_audio_deck;

// Prototypes for clock-slaved subsystems
void LoadWAVMusic(const char* filepath);
void SimulateAudioTelemetry(float dt);

#endif
