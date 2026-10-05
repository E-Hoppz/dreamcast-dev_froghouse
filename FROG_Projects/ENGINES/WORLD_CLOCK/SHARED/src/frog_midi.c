#include "frog_midi.h"
#include "frog_util.h"

int frog_midi_poll(FrogMidiSequence *seq, WorldClock *wc) {
    // Safety check: is the sequencer energized?
    if (!seq || !seq->events || seq->current_cursor >= seq->event_count) {
        return -1;
    }

    FrogMidiEvent *next_event = &seq->events[seq->current_cursor];

    // [THE TRIGGER GATE]
    // Does the World Clock Mechanical Truth match the pre-recorded timestamp?
    if (wc->audio.sample_count >= next_event->timestamp_samples) {
        
        // SHUNT VELOCITY: Use the "Note Hardness" to boost the master peak
        // This allows a MIDI note to physically "Push" an object via the World Clock
        float v_signal = (float)next_event->velocity / 127.0f;
        wc->master_peak = (wc->master_peak * 0.5f) + (v_signal * 0.5f);

        seq->current_cursor++;
        return (int)next_event->note;
    }

    return -1; // Circuit open (No trigger)
}

void frog_midi_reset(FrogMidiSequence *seq) {
    if (seq) {
        seq->current_cursor = 0;
        frog_util_log("MIDI: Sequencer Reset (Loop Triggered).");
    }
}
