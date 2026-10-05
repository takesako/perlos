#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include "heap.h"

extern unsigned char __heap_start[], __heap_end[];

#define RAW_HEAP_START (__heap_start)
#define HEAP_END       (__heap_end)
#define USED           ((size_t)1)
#define ALIGN(n)       (((n) + 7) & ~(size_t)7)

#ifdef HEAP_SMALL_POOL
typedef struct { uint16_t size,n,used; unsigned char *base; void *free; } Pool;
static Pool pool[]={{8,512,0,0,0},{16,768,0,0,0},{24,4640,0,0,0},
                   {32,2640,0,0,0},{48,1200,0,0,0}};
#define NPOOL      (sizeof pool/sizeof *pool)
#define POOL_BYTES ((size_t)(512*8+768*16+4640*24+2640*32+1200*48))
#define HEAP_START (RAW_HEAP_START + POOL_BYTES)
static size_t small_used, small_blocks;
#else
#define POOL_BYTES 0
#define HEAP_START RAW_HEAP_START
#endif
#define CAPACITY ((size_t)(HEAP_END - HEAP_START) & ~(size_t)7)
#define TOTAL    ((size_t)(HEAP_END - RAW_HEAP_START) & ~(size_t)7)

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

#ifdef HEAP_PROFILE
#define HIST_MAX 256
#define HIST_N   (HIST_MAX / 8)
static uint32_t hist[HIST_N], peak_hist[HIST_N], max_hist[HIST_N];
static size_t hist_large, peak_large;
static void hist_add(size_t n){
    if(n<=HIST_MAX){size_t i=n/8-1;if(++hist[i]>max_hist[i])max_hist[i]=hist[i];}
    else hist_large++;
}
static void hist_del(size_t n){if(n<=HIST_MAX)hist[n/8-1]--;else hist_large--;}
static void hist_peak(void){memcpy(peak_hist,hist,sizeof hist);peak_large=hist_large;}
#else
#define hist_add(n) ((void)0)
#define hist_del(n) ((void)0)
#define hist_peak() ((void)0)
#endif

static void init(void)
{
    if (ready) return;
#ifdef HEAP_SMALL_POOL
    unsigned char *p=RAW_HEAP_START;
    for(size_t i=0;i<NPOOL;i++){
        Pool *q=&pool[i]; q->base=p; q->free=0;
        for(unsigned j=0;j<q->n;j++){void *x=p+(size_t)j*q->size;*(void**)x=q->free;q->free=x;}
        p+=(size_t)q->size*q->n;
    }
#endif
    Block *b=(Block *)HEAP_START;
    b->s.flags=CAPACITY-H; b->s.prev=0; blocks=1; ready=1;
}

static Block *next(Block *b)
{
    unsigned char *p=(unsigned char *)(b+1)+SIZE(b);
    return p==HEAP_END?NULL:(Block *)p;
}

static size_t all_used(void)
{
#ifdef HEAP_SMALL_POOL
    return used+small_used;
#else
    return used;
#endif
}

static void peak(void)
{
    size_t u=all_used(), occupied=u+blocks*H;
    if(u>cnt.peak_used)cnt.peak_used=u;
    if(occupied>cnt.peak_occupied){cnt.peak_occupied=occupied;hist_peak();}
}

static void join(Block *b)
{
    Block *q=next(b),*r;
    if(!q||(q->s.flags&USED))return;
    b->s.flags+=H+SIZE(q);blocks--;
    r=next(b);if(r)r->s.prev=H+SIZE(b);
}

static void split(Block *b,size_t n)
{
    size_t old=SIZE(b);if(old-n<H+8)return;
    Block *q=(Block *)((unsigned char *)(b+1)+n),*r;
    b->s.flags=n|(b->s.flags&USED);
    q->s.flags=old-n-H;q->s.prev=H+n;blocks++;
    r=next(q);if(r)r->s.prev=H+SIZE(q);join(q);
}

static void *failure(const char *op,const char *why,size_t n,size_t old,
                     size_t count,size_t elem,void *caller)
{
    cnt.failures++;cnt.last_op=op;cnt.last_reason=why;
    cnt.last_request=n;cnt.last_old_size=old;cnt.last_count=count;
    cnt.last_element=elem;cnt.last_caller=(uintptr_t)caller;errno=ENOMEM;return NULL;
}

#ifdef HEAP_SMALL_POOL
static Pool *small_pool(void *p)
{
    uintptr_t x=(uintptr_t)p;
    for(size_t i=0;i<NPOOL;i++){
        uintptr_t a=(uintptr_t)pool[i].base,b=a+(size_t)pool[i].size*pool[i].n;
        if(x>=a&&x<b)return &pool[i];
    }
    return NULL;
}

static void *small_alloc(size_t n)
{
    for(size_t i=0;i<NPOOL;i++)if(pool[i].size==n&&pool[i].free){
        Pool *q=&pool[i];void *p=q->free;q->free=*(void **)p;q->used++;
        small_used+=n;small_blocks++;hist_add(n);peak();return p;
    }
    return NULL;
}
#endif

static void *allocate(size_t n)
{
    init();
    for(Block *b=(Block *)HEAP_START;b;b=next(b)){
        if((b->s.flags&USED)||SIZE(b)<n)continue;
        split(b,n);b->s.flags|=USED;used+=SIZE(b);hist_add(SIZE(b));peak();return b+1;
    }
    return NULL;
}

static void *alloc_any(size_t n)
{
    init();
#ifdef HEAP_SMALL_POOL
    void *p=small_alloc(n);if(p)return p;
#endif
    return allocate(n);
}

void *malloc(size_t n)
{
    void *p;cnt.malloc_calls++;if(!n)return NULL;
    if(n>(size_t)-1-7)return failure("malloc","size overflow",n,0,1,n,__builtin_return_address(0));
    p=alloc_any(ALIGN(n));
    return p?p:failure("malloc","no fitting block",n,0,1,n,__builtin_return_address(0));
}

void free(void *p)
{
    if(!p)return;
    cnt.free_calls++;
#ifdef HEAP_SMALL_POOL
    Pool *q=small_pool(p);
    if(q){hist_del(q->size);small_used-=q->size;small_blocks--;q->used--;
        *(void **)p=q->free;q->free=p;return;}
#endif
    Block *b=(Block *)p-1;hist_del(SIZE(b));used-=SIZE(b);b->s.flags&=~USED;join(b);
    if(b->s.prev){Block *q=(Block *)((unsigned char *)b-b->s.prev);if(!(q->s.flags&USED))join(q);}
}

void *calloc(size_t n,size_t s)
{
    void *p;size_t z;cnt.calloc_calls++;
    if(n&&s>(size_t)-1/n)return failure("calloc","multiply overflow",(size_t)-1,0,n,s,__builtin_return_address(0));
    z=n*s;if(!z)return NULL;
    if(z>(size_t)-1-7)return failure("calloc","size overflow",z,0,n,s,__builtin_return_address(0));
    p=alloc_any(ALIGN(z));if(p)memset(p,0,z);
    return p?p:failure("calloc","no fitting block",z,0,n,s,__builtin_return_address(0));
}

void *realloc(void *p,size_t n)
{
    void *q;size_t old=0,z;cnt.realloc_calls++;
#ifdef HEAP_SMALL_POOL
    Pool *sp=p?small_pool(p):NULL;
    if(p)old=sp?sp->size:SIZE((Block *)p-1);
#else
    if(p)old=SIZE((Block *)p-1);
#endif
    if(!n){free(p);return NULL;}
    if(n>(size_t)-1-7)return failure("realloc","size overflow",n,old,1,n,__builtin_return_address(0));
    z=ALIGN(n);
    if(p){
#ifdef HEAP_SMALL_POOL
        if(sp){if(z<=old){cnt.realloc_inplace++;return p;}}
        else
#endif
        {
            Block *b=(Block *)p-1,*r=next(b);
            if(old<z&&r&&!(r->s.flags&USED)&&old+H+SIZE(r)>=z)join(b);
            if(SIZE(b)>=z){hist_del(old);split(b,z);hist_add(SIZE(b));
                used=used-old+SIZE(b);peak();cnt.realloc_inplace++;return p;}
        }
    }
    q=alloc_any(z);
    if(!q)return failure("realloc","no fitting block",n,old,1,n,__builtin_return_address(0));
    if(p){memcpy(q,p,old);free(p);cnt.realloc_moved++;}return q;
}

int heap_get_stats(struct heap_stats *s)
{
    size_t off=0,prev=0,live=0,count=0;int was_free=0;
    init();*s=cnt;s->total=TOTAL;s->used=s->free=s->largest=s->overhead=0;
    s->used_blocks=s->free_blocks=0;
    while(off<CAPACITY){
        if(CAPACITY-off<H)return -1;
        Block *b=(Block *)(HEAP_START+off);size_t n=SIZE(b);
        if((b->s.flags&6)||!n||n>CAPACITY-off-H||b->s.prev!=prev)return -1;
        if(b->s.flags&USED){s->used+=n;s->used_blocks++;live+=n;was_free=0;}
        else{if(was_free)return -1;s->free+=n;s->free_blocks++;was_free=1;if(n>s->largest)s->largest=n;}
        count++;prev=H+n;off+=prev;
    }
    s->overhead=count*H;
    if(live!=used||count!=blocks||off!=CAPACITY)return -1;
#ifdef HEAP_SMALL_POOL
    for(size_t i=0;i<NPOOL;i++){
        size_t u=(size_t)pool[i].size*pool[i].used,f=(size_t)pool[i].size*(pool[i].n-pool[i].used);
        s->used+=u;s->free+=f;s->used_blocks+=pool[i].used;
    }
#endif
    return 0;
}

int heap_check(void){struct heap_stats s;return heap_get_stats(&s);}
void heap_reset_peak(void){init();cnt.peak_used=all_used();cnt.peak_occupied=all_used()+blocks*H;hist_peak();}
void heap_failure_site(void *p){cnt.last_caller=(uintptr_t)p;}

#ifdef HEAP_PROFILE
static void dump_hist(const char *tag,const uint32_t *h,size_t large)
{
    size_t n=0,bytes=0;printf("[heap sizes %s]\n",tag);
    for(size_t i=0;i<HIST_N;i++)if(h[i]){size_t z=(i+1)*8;
        printf(" %3zu: %u\n",z,(unsigned)h[i]);n+=h[i];bytes+=z*h[i];}
    printf(" <=%u blocks=%zu bytes=%zu header8=%zu >%u blocks=%zu\n",
           HIST_MAX,n,bytes,n*H,HIST_MAX,large);
}
static void heap_histogram(void){dump_hist("current",hist,hist_large);dump_hist("peak",peak_hist,peak_large);dump_hist("max",max_hist,0);}
#else
#define heap_histogram() ((void)0)
#endif

#ifdef HEAP_SMALL_POOL
static void pool_dump(void)
{
    printf("[pool]");for(size_t i=0;i<NPOOL;i++)printf(" %u:%u/%u",pool[i].size,pool[i].used,pool[i].n);
    printf(" reserved=%zu saved=%zu\n",(size_t)POOL_BYTES,small_blocks*H);
}
#else
#define pool_dump() ((void)0)
#endif

void heap_dump(const char *tag)
{
    struct heap_stats s;int bad=heap_get_stats(&s);
    printf("[heap %s]",tag?tag:"snapshot");if(bad){printf(" CORRUPT\n");return;}
    printf(" total=%zu used=%zu free=%zu largest=%zu overhead=%zu\n",s.total,s.used,s.free,s.largest,s.overhead);
    printf("  occupied=%zu peak_used=%zu peak_occupied=%zu live_blocks=%zu free_blocks=%zu\n",
           s.used+s.overhead,s.peak_used,s.peak_occupied,s.used_blocks,s.free_blocks);
    printf("  malloc=%zu calloc=%zu realloc=%zu inplace=%zu moved=%zu failures=%zu\n",
           s.malloc_calls,s.calloc_calls,s.realloc_calls,s.realloc_inplace,s.realloc_moved,s.failures);
    pool_dump();heap_histogram();
}

void heap_dump_failure(const char *file,unsigned line)
{
    printf("[heap failure] op=%s request=%zu old=%zu count=%zu element=%zu reason=%s\n  caller=0x%zx source=%s:%u\n",
           cnt.last_op?cnt.last_op:"unknown",cnt.last_request,cnt.last_old_size,cnt.last_count,cnt.last_element,
           cnt.last_reason?cnt.last_reason:"unknown",cnt.last_caller,file?file:"(startup)",line);
    heap_dump("oom");
}
