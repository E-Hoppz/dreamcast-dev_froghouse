#ifndef FROG_STATS_H
#define FROG_STATS_H

typedef struct {
    int   score;
    int   combo;
    int   max_combo;
    float accuracy;   // 0.0 to 1.0 (For Rhythm hits or Racing lines)
    float high_score;
} FrogStats;

// Resets stats at the start of a race or a song
void frog_reset_stats(FrogStats *s);

// Adds to the score with a multiplier based on combo
void frog_add_score(FrogStats *s, int points, int multiplier);

#endif
