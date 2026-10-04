#include "frog_stats.h"

void frog_reset_stats(FrogStats *s) {
    s->score = 0;
    s->combo = 0;
    s->max_combo = 0;
    s->accuracy = 1.0f;
}

void frog_add_score(FrogStats *s, int points, int multiplier) {
    s->score += (points * multiplier);
    if(s->combo > s->max_combo) {
        s->max_combo = s->combo;
    }
}
