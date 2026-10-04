#include "frog_data.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void frog_data_load(frog_table_t *table, const char *path) {
    file_t f = fs_open(path, O_RDONLY);
    if (f == FILEHND_INVALID) return;

    table->count = 0;
    char line[128];

    // Simple line-by-line parser for "key=value"
    while (table->count < MAX_ENTRIES && fs_read(f, line, 128) > 0) {
        char *k = strtok(line, "=");
        char *v = strtok(NULL, "\n");
        if (k && v) {
            strncpy(table->entries[table->count].key, k, MAX_STR_LEN);
            strncpy(table->entries[table->count].value, v, MAX_STR_LEN);
            table->count++;
        }
    }
    fs_close(f);
}

float frog_data_get_float(frog_table_t *table, const char *key) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].key, key) == 0) {
            return strtof(table->entries[i].value, NULL);
        }
    }
    return 0.0f;
}

const char* frog_data_get_str(frog_table_t *table, const char *key) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].key, key) == 0) {
            return table->entries[i].value;
        }
    }
    return "UNKNOWN";
}
