#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <errno.h>
#include <sys/types.h>

#define WNOHANG 1
#define WUNTRACED 2
#define WIFEXITED(s) (((s)&127)==0)
#define WEXITSTATUS(s) (((s)>>8)&255)
#define WIFSIGNALED(s) (((s)&127)!=0 && ((s)&127)!=127)
#define WTERMSIG(s) ((s)&127)
#define WIFSTOPPED(s) (((s)&255)==127)
#define WSTOPSIG(s) (((s)>>8)&255)
#define WCOREDUMP(s) ((s)&128)

static inline pid_t wait(int *s){
    (void)s;errno=ECHILD;return -1;
}

static inline pid_t waitpid(pid_t p,int *s,int o){
    (void)p;(void)s;(void)o;errno=ECHILD;return -1;
}

#endif
