#ifndef HOST_COMPAT_DC_SOUND_STREAM_H
#define HOST_COMPAT_DC_SOUND_STREAM_H

#include <stdint.h>
#include <stdlib.h>

typedef int snd_stream_hnd_t;
#define SND_STREAM_INVALID -1
#define SND_STREAM_BUFFER_MAX 65536

static inline int snd_stream_init(void) { return 0; }

static inline snd_stream_hnd_t snd_stream_alloc(void* (*cb)(snd_stream_hnd_t, int, int*), int bufsize) {
    (void)cb; (void)bufsize;
    return 0;
}

static inline void snd_stream_volume(snd_stream_hnd_t hnd, int vol) { (void)hnd; (void)vol; }
static inline void snd_stream_start(snd_stream_hnd_t hnd, uint32_t freq, int stereo) { (void)hnd; (void)freq; (void)stereo; }
static inline void snd_stream_stop(snd_stream_hnd_t hnd) { (void)hnd; }
static inline void snd_stream_poll(snd_stream_hnd_t hnd) { (void)hnd; }

#endif
