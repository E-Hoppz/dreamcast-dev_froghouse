#ifndef FROG_DATA_H
#define FROG_DATA_H

#include <kos.h>

#define MAX_ENTRIES 64
#define MAX_STR_LEN 32

typedef struct {
    char key[MAX_STR_LEN];
    char value[MAX_STR_LEN];
} frog_entry_t;

typedef struct {
    frog_entry_t entries[MAX_ENTRIES];
    int count;
} frog_table_t;

// Load a simple config file (key=value format)
void frog_data_load(frog_table_t *table, const char *path);

// Get a value as a float (for car physics)
float frog_data_get_float(frog_table_t *table, const char *key);

// Get a value as a string (for song titles)
const char* frog_data_get_str(frog_table_t *table, const char *key);

#endif
