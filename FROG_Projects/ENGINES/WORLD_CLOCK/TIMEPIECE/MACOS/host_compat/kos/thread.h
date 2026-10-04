#ifndef HOST_COMPAT_KOS_THREAD_H
#define HOST_COMPAT_KOS_THREAD_H

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

typedef pthread_t kthread_t;

static inline kthread_t* thd_create(int detach, void *(*func)(void *), void *arg) {
    pthread_t *thread = (pthread_t *)malloc(sizeof(pthread_t));
    if (pthread_create(thread, NULL, func, arg) != 0) {
        free(thread);
        return NULL;
    }
    if (detach) {
        pthread_detach(*thread);
    }
    return (kthread_t*)thread;
}

static inline void thd_sleep(unsigned long ms) {
#if defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

#endif
