#ifndef HOST_COMPAT_DBGIO_H
#define HOST_COMPAT_DBGIO_H

#include <stdio.h>
#include <stdint.h>

static inline int dbgio_write_buffer(const uint8_t *data, int len) {
    // Redirect direct SPU/serial writes back to standard output
    return fwrite(data, 1, len, stdout);
}

#endif
