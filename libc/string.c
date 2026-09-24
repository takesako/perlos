#include "string.h"
#include <stdint.h>
#include <errno.h>

size_t strlen(const char*s){const char*p=s;while(*p)p++;return p-s;}

void *memcpy(void*d,const void*s,size_t n){
    unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;
}

void *memmove(void*d,const void*s,size_t n){
    unsigned char*a=d;const unsigned char*b=s;
    if((uintptr_t)a<(uintptr_t)b)while(n--)*a++=*b++;
    else if((uintptr_t)a>(uintptr_t)b){a+=n;b+=n;while(n--)*--a=*--b;}
    return d;
}

void *memset(void*d,int c,size_t n){
    unsigned char*p=d;while(n--)*p++=(unsigned char)c;return d;
}

int memcmp(const void*a,const void*b,size_t n){
    const unsigned char*p=a,*q=b;
    while(n--){if(*p!=*q)return *p-*q;p++;q++;}return 0;
}

void *memchr(const void*s,int c,size_t n){
    const unsigned char*p=s;while(n--)if(*p++==(unsigned char)c)return(void*)(p-1);return 0;
}

char *strcpy(char*d,const char*s){char*r=d;while((*d++=*s++));return r;}

char *strncpy(char*d,const char*s,size_t n){
    char*r=d;while(n&&*s){*d++=*s++;n--;}while(n--)*d++=0;return r;
}

char *strcat(char*d,const char*s){
    char*r=d;while(*d)d++;while((*d++=*s++));return r;
}

int strcmp(const char*a,const char*b){
    while(*a&&*a==*b)a++,b++;return(unsigned char)*a-(unsigned char)*b;
}

int strncmp(const char*a,const char*b,size_t n){
    while(n&&*a&&*a==*b)a++,b++,n--;return n?(unsigned char)*a-(unsigned char)*b:0;
}

char *strchr(const char*s,int c){
    for(;;s++){if(*s==(char)c)return(char*)s;if(!*s)return 0;}
}

char *strrchr(const char*s,int c){
    const char*r=0;do{if(*s==(char)c)r=s;}while(*s++);return(char*)r;
}

char *strstr(const char*s,const char*f){
    size_t n=strlen(f);if(!n)return(char*)s;
    for(;*s;s++)if(*s==*f&&!strncmp(s,f,n))return(char*)s;
    return 0;
}

size_t strspn(const char *s, const char *set)
{
    const char *p = s; while (*p && strchr(set, *p)) p++; return (size_t)(p-s);
}
size_t strcspn(const char *s, const char *set)
{
    const char *p = s; while (*p && !strchr(set, *p)) p++; return (size_t)(p-s);
}
char *strpbrk(const char *s, const char *set)
{
    s += strcspn(s, set); return *s ? (char *)s : 0;
}
char *strncat(char *d, const char *s, size_t n)
{
    char *p = d+strlen(d); while (n-- && *s) *p++ = *s++; *p = 0; return d;
}
char *strerror(int n)
{
    switch (n) {
    case 0: return "Success";
    case EPERM: return "Operation not permitted";
    case ENOENT: return "No such file or directory";
    case EIO: return "Input/output error";
    case EBADF: return "Bad file descriptor";
    case ECHILD: return "No child processes";
    case ENOMEM: return "Out of memory";
    case EACCES: return "Permission denied";
    case EEXIST: return "File exists";
    case ENOTDIR: return "Not a directory";
    case EISDIR: return "Is a directory";
    case EINVAL: return "Invalid argument";
    case EMFILE: return "Too many open files";
    case ENOTTY: return "Not a terminal";
    case ESPIPE: return "Illegal seek";
    case EROFS: return "Read-only file system";
    case ERANGE: return "Result out of range";
    case EDOM: return "Numerical argument out of domain";
    case ENOSYS: return "Function not implemented";
    case EOVERFLOW: return "Value too large";
    case ENAMETOOLONG: return "File name too long";
    default: return "Unknown error";
    }
}
