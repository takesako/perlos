#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include "io.h"
#include "timer.h"

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

extern char **environ;

ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);
off_t lseek(int, off_t, int);

int close(int);
int dup(int);
int dup2(int, int);

int unlink(const char *);
int link(const char *, const char *);
int chdir(const char *);
char *getcwd(char *, size_t);
int access(const char *, int);
int isatty(int);

static inline pid_t getpid(void){return 1;}
static inline pid_t getppid(void){return 0;}
static inline uid_t getuid(void){return 0;}
static inline uid_t geteuid(void){return 0;}
static inline gid_t getgid(void){return 0;}
static inline gid_t getegid(void){return 0;}

static inline int setuid(uid_t x){
    (void)x;errno=ENOSYS;return -1;
}

static inline int setgid(gid_t x){
    (void)x;errno=ENOSYS;return -1;
}

static inline pid_t fork(void){
    errno=ENOSYS;return -1;
}

static inline int pipe(int p[2]){
    (void)p;errno=ENOSYS;return -1;
}

static inline int execv(const char *p,char *const a[]){
    (void)p;(void)a;errno=ENOSYS;return -1;
}

static inline int execvp(const char *p,char *const a[]){
    return execv(p,a);
}

static inline int execl(const char *p,const char *a,...){
    (void)p;(void)a;errno=ENOSYS;return -1;
}

static inline unsigned sleep(unsigned s){
    while(s--)msleep(1000);
    return 0;
}

#define _exit(n) _io_exit(n)

#endif
