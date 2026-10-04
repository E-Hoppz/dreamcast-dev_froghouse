
#ifndef FROG_MIDI_H
#define FROG_MIDI_H

#include <stdint.h>
#include "world_clock.h"

// [REPAIRMAN'S SPEC]
// A single timed trigger from your DAW
typedef struct {
    uint32_t timestamp_samples; // AICA sample point
    uint8_t  note;              // The "Gate ID" (0-127)
    uint8_t  velocity;          // The "Voltage" (0-127)
} FrogMidiEvent;

typedef struct {
    FrogMidiEvent *events;
    uint32_t event_count;
    uint32_t current_cursor;
} FrogMidiSequence;

/**
 * [THE SEQUENCER POLL]
 * Checks the MIDI rail. Returns the Note ID if a trigger hits.
 * Also shunts the Velocity into wc->master_peak for instant physics reaction.
 */
int frog_midi_poll(FrogMidiSequence *seq, WorldClock *wc);

/**
 * [THE RESET RELAY]
 * Jumps the cursor back to the start. Call this when the Mixtape loops.
 */
void frog_midi_reset(FrogMidiSequence *seq);

#endif
