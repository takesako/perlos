#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include "heap.h"

extern unsigned char __heap_start[];
extern unsigned char __heap_end[];

#define HEAP_START (__heap_start)
#define HEAP_END   (__heap_end)
#define CAPACITY   ((size_t)(HEAP_END - HEAP_START) & ~(size_t)7)
#define USED       ((size_t)1)
#define ALIGN(n)   (((n) + 7) & ~(size_t)7)

typedef union Block {
    struct { size_t flags, prev; } s;
    unsigned long long align;
} Block;

#define H       sizeof(Block)
#define SIZE(b) ((b)->s.flags & ~(size_t)7)

_Static_assert(sizeof(Block) % 8 == 0, "heap header alignment");

static int ready;
static size_t used, blocks;
static struct heap_stats cnt;

static void init(void)
{
    if (ready) return;
    Block *b = (Block *)HEAP_START;
    b->s.flags = CAPACITY - H;
    b->s.prev = 0;
    blocks = 1; ready = 1;
}

static Block *next(Block *b)
{
    unsigned char *p = (unsigned char *)(b + 1) + SIZE(b);
    return p == HEAP_END ? NULL : (Block *)p;
}

static void peak(void)
{
    size_t occupied = used + blocks * H;
    if (used > cnt.peak_used) cnt.peak_used = used;
    if (occupied > cnt.peak_occupied) cnt.peak_occupied = occupied;
}

static void join(Block *b)
{
    Block *q = next(b), *r;
    if (!q || (q->s.flags & USED)) return;
    b->s.flags += H + SIZE(q); blocks--;
    r = next(b); if (r) r->s.prev = H + SIZE(b);
}

static void split(Block *b, size_t n)
{
    size_t old = SIZE(b);
    if (old - n < H + 8) return;
    Block *q = (Block *)((unsigned char *)(b + 1) + n), *r;
    b->s.flags = n | (b->s.flags & USED);
    q->s.flags = old - n - H; q->s.prev = H + n; blocks++;
    r = next(q); if (r) r->s.prev = H + SIZE(q);
    join(q);
}

static void *failure(const char *op, const char *why, size_t n,
                     size_t old, size_t count, size_t elem, void *caller)
{
    cnt.failures++;
    cnt.last_op = op; cnt.last_reason = why;
    cnt.last_request = n; cnt.last_old_size = old;
    cnt.last_count = count; cnt.last_element = elem;
    cnt.last_caller = (uintptr_t)caller;
    errno = ENOMEM;
    return NULL;
}

static void *allocate(size_t n)
{
    init();
    for (Block *b = (Block *)HEAP_START; b; b = next(b)) {
        if ((b->s.flags & USED) || SIZE(b) < n) continue;
        split(b, n); b->s.flags |= USED; used += SIZE(b); peak();
        return b + 1;
    }
    return NULL;
}

void *malloc(size_t n)
{
    void *p; cnt.malloc_calls++;
    if (!n) return NULL;
    if (n > (size_t)-1 - 7)
        return failure("malloc", "size overflow", n, 0, 1, n,
                       __builtin_return_address(0));
    p = allocate(ALIGN(n));
    return p ? p : failure("malloc", "no fitting block", n, 0, 1, n,
                            __builtin_return_address(0));
}

void free(void *p)
{
    if (!p) return;
    cnt.free_calls++;
    Block *b = (Block *)p - 1;
    used -= SIZE(b); b->s.flags &= ~USED;
    join(b);
    if (b->s.prev) {
        Block *q = (Block *)((unsigned char *)b - b->s.prev);
        if (!(q->s.flags & USED)) join(q);
    }
}

void *calloc(size_t n, size_t s)
{
    void *p; size_t z; cnt.calloc_calls++;
    if (n && s > (size_t)-1 / n)
        return failure("calloc", "multiply overflow", (size_t)-1, 0, n, s,
                       __builtin_return_address(0));
    z = n * s;
    if (!z) return NULL;
    if (z > (size_t)-1 - 7)
        return failure("calloc", "size overflow", z, 0, n, s,
                       __builtin_return_address(0));
    p = allocate(ALIGN(z));
    if (p) memset(p, 0, z);
    return p ? p : failure("calloc", "no fitting block", z, 0, n, s,
                           __builtin_return_address(0));
}

void *realloc(void *p, size_t n)
{
    void *q; size_t old = p ? SIZE((Block *)p - 1) : 0;
    cnt.realloc_calls++;
    if (!n) { free(p); return NULL; }
    if (n > (size_t)-1 - 7)
        return failure("realloc", "size overflow", n, old, 1, n,
                       __builtin_return_address(0));
    size_t z = ALIGN(n);
    if (p) {
        Block *b = (Block *)p - 1, *r = next(b);
        if (old < z && r && !(r->s.flags & USED) &&
            old + H + SIZE(r) >= z)
            join(b);
        if (SIZE(b) >= z) {
            split(b, z); used = used - old + SIZE(b); peak();
            cnt.realloc_inplace++; return p;
        }
    }
    q = allocate(z);
    if (!q)
        return failure("realloc", "no fitting block", n, old, 1, n,
                       __builtin_return_address(0));
    if (p) { memcpy(q, p, old); free(p); cnt.realloc_moved++; }
    return q;
}

int heap_get_stats(struct heap_stats *s)
{
    size_t off = 0, prev = 0, live = 0, count = 0;
    int was_free = 0;
    init(); *s = cnt; s->total = CAPACITY;
    s->used = s->free = s->largest = s->overhead = 0;
    s->used_blocks = s->free_blocks = 0;
    while (off < CAPACITY) {
        if (CAPACITY - off < H) return -1;
        Block *b = (Block *)(HEAP_START + off);
        size_t n = SIZE(b);
        if ((b->s.flags & 6) || !n ||
            n > CAPACITY - off - H || b->s.prev != prev)
            return -1;
        if (b->s.flags & USED) {
            s->used += n; s->used_blocks++; live += n; was_free = 0;
        } else {
            if (was_free) return -1;
            s->free += n; s->free_blocks++; was_free = 1;
            if (n > s->largest) s->largest = n;
        }
        count++; prev = H + n; off += prev;
    }
    s->overhead = count * H;
    return live == used && count == blocks && off == CAPACITY ? 0 : -1;
}

int heap_check(void)
{
    struct heap_stats s;
    return heap_get_stats(&s);
}

void heap_reset_peak(void)
{
    init();
    cnt.peak_used = used;
    cnt.peak_occupied = used + blocks * H;
}

void heap_failure_site(void *p)
{
    cnt.last_caller = (uintptr_t)p;
}

void heap_dump(const char *tag)
{
    struct heap_stats s;
    int bad = heap_get_stats(&s);
    printf("[heap %s]", tag ? tag : "snapshot");
    if (bad) { printf(" CORRUPT\n"); return; }
    printf(" total=%zu used=%zu free=%zu largest=%zu overhead=%zu\n",
           s.total, s.used, s.free, s.largest, s.overhead);
    printf("  occupied=%zu peak_used=%zu peak_occupied=%zu "
           "live_blocks=%zu free_blocks=%zu\n",
           s.used + s.overhead, s.peak_used, s.peak_occupied,
           s.used_blocks, s.free_blocks);
    printf("  malloc=%zu calloc=%zu realloc=%zu inplace=%zu "
           "moved=%zu failures=%zu\n",
           s.malloc_calls, s.calloc_calls, s.realloc_calls,
           s.realloc_inplace, s.realloc_moved, s.failures);
}

void heap_dump_failure(const char *file, unsigned line)
{
    printf("[heap failure] op=%s request=%zu old=%zu count=%zu element=%zu "
           "reason=%s\n  caller=0x%zx source=%s:%u\n",
           cnt.last_op ? cnt.last_op : "unknown",
           cnt.last_request, cnt.last_old_size,
           cnt.last_count, cnt.last_element,
           cnt.last_reason ? cnt.last_reason : "unknown",
           cnt.last_caller,
           file ? file : "(startup)", line);
    heap_dump("oom");
}
