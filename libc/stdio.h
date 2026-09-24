#ifndef _STDIO_H
#define _STDIO_H
#include <stddef.h>
#include <stdarg.h>
#include "io.h"
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define BUFSIZ 1024
#define FILENAME_MAX 256
#define FOPEN_MAX 32
#define L_tmpnam 32
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

typedef struct {
    const unsigned char *p;
    size_t size, pos;
    int push;
} FILE;

typedef long fpos_t;

extern FILE _stdin, _stdout, _stderr;
#define stdin (&_stdin)
#define stdout (&_stdout)
#define stderr (&_stderr)
#define getc(f) fgetc(f)
#define putc(c, f) fputc(c, f)
FILE *fopen(const char *, const char *);
FILE *freopen(const char *, const char *, FILE *);
FILE *fdopen(int, const char *);
FILE *tmpfile(void);
int fclose(FILE *);
int fflush(FILE *);
int fileno(FILE *);
int feof(FILE *);
int ferror(FILE *);
void clearerr(FILE *);
void rewind(FILE *);
void setbuf(FILE *, char *);
int setvbuf(FILE *, char *, int, size_t);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int fgetc(FILE *);
int fputc(int, FILE *);
int ungetc(int, FILE *);
char *fgets(char *, int, FILE *);
int fputs(const char *, FILE *);
int puts(const char *);
int fseek(FILE *, long, int);
long ftell(FILE *);
int fgetpos(FILE *, fpos_t *);
int fsetpos(FILE *, const fpos_t *);
int remove(const char *);
int rename(const char *, const char *);
void perror(const char *);
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int sprintf(char *, const char *, ...);
int snprintf(char *, size_t, const char *, ...);
int vprintf(const char *, va_list);
int vfprintf(FILE *, const char *, va_list);
int vsprintf(char *, const char *, va_list);
int vsnprintf(char *, size_t, const char *, va_list);
#endif
