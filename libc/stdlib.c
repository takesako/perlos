#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#ifndef LIBC_HEAP_SIZE
#define LIBC_HEAP_SIZE (128 * 1024)
#endif

#define ALIGN(n) (((n) + 7) & ~(size_t)7)

typedef union Block Block;

union Block {
    struct {
        size_t size;
        Block *next;
        int free;
    } s;
    long long align;
};

static union {
    long long align;
    unsigned char data[LIBC_HEAP_SIZE];
} heap;

static Block *head;
static unsigned int rnd = 1;

static void init(void)
{
    if (head) return;
    head = (Block *)heap.data;
    head->s.size = LIBC_HEAP_SIZE - sizeof(Block);
    head->s.next = NULL;
    head->s.free = 1;
}

static void merge(void)
{
    Block *p;
    for (p = head; p && p->s.next;) {
        if (p->s.free && p->s.next->s.free) {
            p->s.size += sizeof(Block) + p->s.next->s.size;
            p->s.next = p->s.next->s.next;
        } else {
            p = p->s.next;
        }
    }
}

void *malloc(size_t n)
{
    Block *p, *q;
    if (!n) return NULL;
    if (n > (size_t)-1-7) { errno = ENOMEM; return NULL; }
    init();
    n = ALIGN(n);
    for (p = head; p; p = p->s.next) {
        if (!p->s.free || p->s.size < n) continue;
        if (p->s.size - n >= sizeof(Block) + 8) {
            q = (Block *)((unsigned char *)(p + 1) + n);
            q->s.size = p->s.size - n - sizeof(Block);
            q->s.next = p->s.next;
            q->s.free = 1;
            p->s.next = q;
            p->s.size = n;
        }
        p->s.free = 0;
        return p + 1;
    }
    errno = ENOMEM; return NULL;
}

void free(void *ptr)
{
    Block *p;
    if (!ptr) return;
    p = (Block *)ptr - 1;
    p->s.free = 1;
    merge();
}

void *calloc(size_t n, size_t size)
{
    size_t total; void *p;
    if (n && size > (size_t)-1 / n) { errno = ENOMEM; return NULL; }
    total = n * size;
    p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t n)
{
    Block *b; void *p;
    if (!ptr) return malloc(n);
    if (!n) { free(ptr); return NULL; }
    b = (Block *)ptr - 1;
    if (b->s.size >= n) return ptr;
    p = malloc(n);
    if (!p) return NULL;
    memcpy(p, ptr, b->s.size);
    free(ptr);
    return p;
}

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
