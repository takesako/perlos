#ifndef _STDLIB_H
#define _STDLIB_H

#include <stdio.h>
#include <stddef.h>
#include <errno.h>

#define RAND_MAX 2147483647

void srand(unsigned int);
int rand(void);

void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);

void qsort(void *,size_t,size_t,int (*)(const void *,const void *));

static inline char *getenv(const char *s) { (void)s; return 0; }
static inline int putenv(char *s) { (void)s; errno = ENOSYS; return -1; }

unsigned long strtoul(const char *,char **,int);

int atoi(const char *);
long atol(const char *);
long strtol(const char *,char **,int);
int abs(int);
long labs(long);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

static inline __attribute__((noreturn)) void abort(void) {_io_exit(134);}
static inline __attribute__((noreturn)) void exit(int n) {fflush(0);_io_exit(n);}
static inline int system(const char *s) {if(!s)return 0;errno=ENOSYS;return-1;}
static inline int atexit(void (*f)(void)) {(void)f;errno=ENOMEM;return-1;}

typedef struct { int quot, rem; } div_t;

div_t div(int, int);
unsigned long long strtoull(const char *, char **, int);
long long strtoll(const char *, char **, int);

#endif
