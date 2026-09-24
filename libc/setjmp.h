#ifndef _SETJMP_H
#define _SETJMP_H

typedef union {
    unsigned int r[26];
    unsigned long long align;
} __jmp_buf;

typedef __jmp_buf jmp_buf[1];

int setjmp(jmp_buf) __attribute__((returns_twice));
void longjmp(jmp_buf, int) __attribute__((noreturn));

#endif
