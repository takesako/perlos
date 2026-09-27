#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

#include <sys/time.h>
#include <stddef.h>
#include "timer.h"

#define FD_SETSIZE 64

typedef struct {
    unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long)) + 1];
} fd_set;

static inline int select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t) {
    (void)n; (void)r; (void)w; (void)e;
    if (t) msleep((t->tv_sec * 1000) + (t->tv_usec / 1000));
    return 0;
}

#endif
