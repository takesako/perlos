#ifndef _DIRENT_H
#define _DIRENT_H

#include <errno.h>

typedef struct { int unused; } DIR;
struct dirent { unsigned long d_ino; char d_name[256]; };

static inline DIR *opendir(const char *p){(void)p;errno=ENOSYS;return 0;}
static inline struct dirent *readdir(DIR *p){(void)p;errno=ENOSYS;return 0;}
static inline int closedir(DIR *p){(void)p;errno=ENOSYS;return -1;}
static inline void rewinddir(DIR *p){(void)p;errno=ENOSYS;}
static inline long telldir(DIR *p){(void)p;errno=ENOSYS;return -1;}
static inline void seekdir(DIR *p,long n){(void)p;(void)n;errno=ENOSYS;}

#endif
