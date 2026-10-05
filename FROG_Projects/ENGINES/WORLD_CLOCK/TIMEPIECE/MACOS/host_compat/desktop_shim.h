#ifndef DESKTOP_SHIM_H
#define DESKTOP_SHIM_H

// --- [ AUTOMATIC C-LINKAGE GUARDS FOR CROSS-LANGUAGE SAFETY ] ---
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef int file_t;
#define FILEHND_INVALID -1

static inline file_t fs_open(const char *fn, int mode) {
    const char *real_fn = fn;
    if (strncmp(fn, "/vmu/a1/", 8) == 0) real_fn = fn + 8;
    else if (strncmp(fn, "/vmu/", 5) == 0) real_fn = fn + 5;
    
    if (mode & O_CREAT) {
        return open(real_fn, mode, 0666);
    }
    return open(real_fn, mode);
}

static inline void fs_close(file_t f) {
    if (f != FILEHND_INVALID) close(f);
}

static inline ssize_t fs_read(file_t f, void *buf, size_t size) {
    return read(f, buf, size);
}

static inline ssize_t fs_write(file_t f, const void *buf, size_t size) {
    return write(f, buf, size);
}

static inline off_t fs_seek(file_t f, off_t offset, int whence) {
    return lseek(f, offset, whence);
}

static inline size_t fs_total(file_t f) {
    struct stat st;
    if (fstat(f, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

// Redirects matrix math cleanly under C-linkage
#include <dc/matrix.h>

#ifdef __cplusplus
}
#endif

#endif
