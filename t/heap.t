#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "heap.h"

static int no, fail;
#define ok(c,n) do { no++; if(c) printf("ok %d - %s\n",no,n); \
else { printf("not ok %d - %s\n",no,n); fail++; } } while(0)
static int filled(const unsigned char *p, size_t n, unsigned char c)
{ while(n--) if(*p++ != c) return 0; return 1; }

int main(void)
{
    struct heap_stats s, t;
    puts("1..20");
    heap_get_stats(&s);
    ok(!heap_check() && !s.used && s.free+s.overhead==s.total,"initial heap");
    void *z=malloc(0); free(NULL);
    ok(z==NULL,"zero size and free(NULL)");
    unsigned char *a=malloc(24), *b=malloc(32), *c=malloc(40);
    ok(a && b && c && !((uintptr_t)a&7) && !((uintptr_t)b&7),"8-byte alignment");
    memset(a,0x42,24); free(b);
    unsigned char *p=realloc(a,56);
    ok(p==a && filled(p,24,0x42),"grow into free right neighbour");
    a=p; memset(a,0x53,56); heap_get_stats(&s);
    p=realloc(a,8); heap_get_stats(&t);
    ok(p==a && t.used<s.used && !heap_check(),"shrink returns tail to heap");
    a=p; b=malloc(16);
    ok(b && b<c && !heap_check(),"reuse shrunk tail");
    p=realloc(a,256);
    ok(p && p!=a && filled(p,8,0x53),"moving realloc preserves contents");
    a=p; memset(a,0x64,256);
    p=realloc(a,(size_t)-8);
    ok(!p && filled(a,256,0x64) && !heap_check(),"failed realloc preserves original");
    p=realloc(a,(size_t)-1);
    ok(!p && filled(a,256,0x64),"realloc overflow preserves original");
    ok(malloc((size_t)-1)==NULL,"malloc rounding overflow");
    ok(calloc((size_t)-1,2)==NULL,"calloc multiplication overflow");
    heap_get_stats(&t);
    ok(t.last_count==(size_t)-1 && t.last_element==2 && t.failures>=4,"failure diagnostics recorded");
    p=calloc(31,3);
    ok(p && filled(p,93,0),"calloc clears payload");
    free(p); free(a); free(c); free(b); heap_get_stats(&t);
    ok(!t.used && t.free_blocks==1 && !heap_check(),"both-sided coalescing");
    p=malloc(17); void *q=realloc(p,0);
    ok(q==NULL && !heap_check(),"realloc zero frees");
    heap_reset_peak(); heap_get_stats(&s); p=malloc(123); heap_get_stats(&t);
    ok(t.peak_occupied>=t.used+t.overhead && t.peak_occupied>s.peak_occupied,"peak accounting");
    free(p); heap_reset_peak(); heap_get_stats(&t);
    ok(t.peak_used==0 && t.peak_occupied==t.overhead,"reset peak");
    void *slots[64]={0}; unsigned sizes[64]={0}; int valid=1;
    srand(42);
    for(unsigned step=0;step<20000 && valid;step++) {
        unsigned i=(unsigned)rand()%64, n=(unsigned)rand()%2048;
        if(slots[i] && !filled(slots[i],sizes[i],(unsigned char)i)) { valid=0; break; }
        p=realloc(slots[i],n);
        if(!n) { slots[i]=NULL; sizes[i]=0; }
        else if(p) {
            unsigned keep=sizes[i]<n?sizes[i]:n;
            if(slots[i] && !filled(p,keep,(unsigned char)i)) valid=0;
            slots[i]=p; sizes[i]=n; memset(p,i,n);
        }
        if(heap_check()) valid=0;
    }
    ok(valid,"20000 randomized operations preserve data and metadata");
    for(unsigned i=0;i<64;i++) free(slots[i]);
    heap_get_stats(&t);
    ok(!heap_check() && !t.used && t.free_blocks==1 && t.free+t.overhead==t.total,"stress cleanup");
    p=malloc(t.largest);
    ok(p && !heap_check(),"allocate entire largest free block");
    free(p);
    return fail!=0;
}
