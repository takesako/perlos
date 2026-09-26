#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

static unsigned int rnd = 1;

void srand(unsigned int seed) { rnd = seed; }

int rand(void)
{
    rnd = rnd * 1103515245u + 12345u;
    return (int)(rnd >> 1);
}

typedef unsigned U32 __attribute__((may_alias));

void qsort(void *p, size_t n, size_t s, int (*cmp)(const void *, const void *))
{
    unsigned char *a = p;
    if (!s || n < 2 || n > (size_t)-1/s) return;
    for (size_t i = 1; i < n; i++)
        for (unsigned char *q = a+i*s; q > a && cmp(q-s, q) > 0; q -= s)
            if (s == 4 && !((uintptr_t)q & 3)) {
                U32 t = *(U32 *)(q-s); *(U32 *)(q-s) = *(U32 *)q; *(U32 *)q = t;
            } else for (size_t j = 0; j < s; j++) {
                unsigned char t = (q-s)[j]; (q-s)[j] = q[j]; q[j] = t;
            }
}

static int digit(unsigned char c)
{
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'a' && c <= 'z') return c-'a'+10;
    if (c >= 'A' && c <= 'Z') return c-'A'+10;
    return 99;
}

static unsigned long long number(const char *s, char **end, int base,
    int *negative, unsigned long long limit, int signed_value, int *overflow)
{
    const char *original = s; unsigned long long v = 0; int any = 0;
    *negative = *overflow = 0;
    if (base && (base < 2 || base > 36)) {
        errno = EINVAL; if(end) *end = (char *)s; return 0;
    }
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;
    if (*s == '-' || *s == '+') *negative = *s++ == '-';
    if ((!base || base == 16) && s[0] == '0' && (s[1] == 'x' || s[1] == 'X') && digit(s[2]) < 16)
        base = 16, s += 2;
    if (!base) base = *s == '0' ? 8 : 10;
    if (signed_value && *negative) limit++;
    for (int d; (d = digit((unsigned char)*s)) < base; s++) {
        any = 1;
        if (v > limit/(unsigned)base || (v == limit/(unsigned)base && (unsigned)d > limit%(unsigned)base))
            *overflow = 1;
        else if (!*overflow) v = v*(unsigned)base+(unsigned)d;
    }
    if (end) *end = (char *)(any ? s : original);
    if (*overflow) { errno = ERANGE; return limit; }
    return v;
}

unsigned long strtoul(const char *s, char **e, int b)
{
    int neg, over; unsigned long v = (unsigned long)number(s,e,b,&neg,ULONG_MAX,0,&over);
    return neg && !over ? 0UL-v : v;
}

long strtol(const char *s, char **e, int b)
{
    int neg, over; unsigned long v = (unsigned long)number(s,e,b,&neg,LONG_MAX,1,&over);
    if (over) return neg ? LONG_MIN : LONG_MAX;
    return neg ? (v == (unsigned long)LONG_MAX+1 ? LONG_MIN : -(long)v) : (long)v;
}

unsigned long long strtoull(const char *s, char **e, int b)
{
    int neg, over; unsigned long long v = number(s,e,b,&neg,ULLONG_MAX,0,&over);
    return neg && !over ? 0ULL-v : v;
}

long long strtoll(const char *s, char **e, int b)
{
    int neg, over; unsigned long long v = number(s,e,b,&neg,LLONG_MAX,1,&over);
    if (over) return neg ? LLONG_MIN : LLONG_MAX;
    return neg ? (v == (unsigned long long)LLONG_MAX+1 ? LLONG_MIN : -(long long)v) : (long long)v;
}

int atoi(const char *s) { return (int)strtol(s, 0, 10); }
long atol(const char *s) { return strtol(s, 0, 10); }
int abs(int n) { return n < 0 ? -n : n; }
long labs(long n) { return n < 0 ? -n : n; }
div_t div(int n, int d) { return (div_t){n/d, n%d}; }
