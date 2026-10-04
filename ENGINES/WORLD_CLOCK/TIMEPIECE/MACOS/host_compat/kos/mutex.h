#ifndef HOST_COMPAT_KOS_MUTEX_H
#define HOST_COMPAT_KOS_MUTEX_H

#include <pthread.h>

typedef pthread_mutex_t mutex_t;
#define MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline int mutex_lock(mutex_t *m) {
    return pthread_mutex_lock(m);
}

static inline int mutex_unlock(mutex_t *m) {
    return pthread_mutex_unlock(m);
}

#endif
