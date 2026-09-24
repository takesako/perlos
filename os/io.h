#ifndef _IO_H
#define _IO_H

void _io_init(void);
__attribute__((noreturn)) void _io_exit(int);

int putchar(int);
int getchar(void);
int getchar_timeout(unsigned);
int console_write(const void *, unsigned);

#endif
