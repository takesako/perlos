// ARM compiler ABI helpers without -lgcc, Treat double as float.
#if defined(__arm__)

#include <string.h>

void __aeabi_memcpy(void *d, const void *s, size_t n) { memcpy(d, s, n); }
void __aeabi_memcpy4(void *, const void *, size_t) __attribute__((alias("__aeabi_memcpy")));
void __aeabi_memcpy8(void *, const void *, size_t) __attribute__((alias("__aeabi_memcpy")));
void __aeabi_memmove(void *d, const void *s, size_t n) { memmove(d, s, n); }
void __aeabi_memmove4(void *, const void *, size_t) __attribute__((alias("__aeabi_memmove")));
void __aeabi_memmove8(void *, const void *, size_t) __attribute__((alias("__aeabi_memmove")));
void __aeabi_memset(void *d, size_t n, int c) { memset(d, c, n); }
void __aeabi_memset4(void *, size_t, int) __attribute__((alias("__aeabi_memset")));
void __aeabi_memset8(void *, size_t, int) __attribute__((alias("__aeabi_memset")));
void __aeabi_memclr(void *d, size_t n) { memset(d, 0, n); }
void __aeabi_memclr4(void *, size_t) __attribute__((alias("__aeabi_memclr")));
void __aeabi_memclr8(void *, size_t) __attribute__((alias("__aeabi_memclr")));

unsigned long long u64div(unsigned long long n, unsigned long long d, unsigned long long *r) {
    unsigned long long q=0,x=0; int i=64;
    if(!d){*r=n;return ~0ULL;}
    while(i--){unsigned c=x>>63;x=(x<<1)|(n>>63);n<<=1;q<<=1;if(c||x>=d)x-=d,q++;}
    *r=x; return q;
}

long long s64div(long long n, long long d, long long *r) {
    unsigned long long a=n,b=d,m,q; int sn=n<0,sd=d<0;
    if(sn)a=0-a; if(sd)b=0-b;
    q=u64div(a,b,&m);
    if(sn)m=0-m; if(sn^sd)q=0-q;
    *r=(long long)m; return (long long)q;
}

__attribute__((naked))
void __aeabi_uldivmod(void){
 __asm volatile("sub ip,sp,#8\nstrd ip,lr,[sp,#-16]!\nbl u64div\nldr lr,[sp,#4]\nldrd r2,r3,[sp,#8]\nadd sp,#16\nbx lr");
}
__attribute__((naked))
void __aeabi_ldivmod(void){
 __asm volatile("sub ip,sp,#8\nstrd ip,lr,[sp,#-16]!\nbl s64div\nldr lr,[sp,#4]\nldrd r2,r3,[sp,#8]\nadd sp,#16\nbx lr");
}

typedef union { unsigned u; float f; } F32;
typedef union { unsigned long long u; double d; struct {unsigned l, h;} w;} D64;

float  __attribute__((pcs("aapcs"))) __aeabi_l2f(long long x) {
    unsigned long long a=x,t; unsigned s=0,g=0,st=0; int e=0,k;
    if(x<0)s=1u<<31,a=0-a; if(!a)return 0;
    for(t=a;t>>=1;e++); k=e;
    if(k>23)while(k-->23){st|=g;g=a&1;a>>=1;}else while(k++<23)a<<=1;
    if(g&&(st||(a&1)))if(++a==(1ULL<<24))a>>=1,e++;
    F32 v={s|((unsigned)(e+127)<<23)|((unsigned)a&0x7fffff)};
    return v.f;
}

long long  __attribute__((pcs("aapcs"))) __aeabi_f2lz(float x) {
    F32 v={.f=x}; unsigned long long a=(v.u&0x7fffff)|0x800000; int e=((v.u>>23)&255)-127;
    if(e<0)return 0;
    if(e>62)return (long long)(v.u>>31?1ULL<<63:(1ULL<<63)-1);
    if(e>23)while(e-->23)a<<=1;else while(e++<23)a>>=1;
    return v.u>>31?-(long long)a:(long long)a;
}

// ABI retains double; but computed as float

static unsigned long long rsh(unsigned long long x, int n) {
    unsigned long long q=x>>n,r=x&((1ULL<<n)-1),h=1ULL<<(n-1);
    return q+(r>h||(r==h&&(q&1)));
}

float __attribute__((pcs("aapcs"))) __aeabi_d2f(double x) {
    D64 d={.d=x}; F32 f; unsigned long long a=d.u,m=a&0xfffffffffffffULL;
    int e=(a>>52)&2047,E=e-1023; unsigned s=a>>32&0x80000000;
    if(e==2047){f.u=s|0x7f800000|(m?0x400000:0);return f.f;}
    if(!e||E<-149){f.u=s;return f.f;}
    if(E>127){f.u=s|0x7f800000;return f.f;}
    m|=1ULL<<52;
    if(E<-126){f.u=s|(unsigned)rsh(m,-E-97);return f.f;}
    m=rsh(m,29);if(m>>24)m>>=1,E++;
    f.u=s|(E>127?0x7f800000:((E+127)<<23)|((unsigned)m&0x7fffff));
    return f.f;
}

double __attribute__((pcs("aapcs"))) __aeabi_f2d(float f) {
    F32 a={.f=f};D64 d; unsigned s=a.u&0x80000000,e=a.u>>23&255,m=a.u&0x7fffff,p; d.w.l=0;
    if(e==255)d.w.h=s|0x7ff00000|(m>>3),d.w.l=m<<29;
    else if(e)d.w.h=s|((e+896)<<20)|(m>>3),d.w.l=m<<29;
    else if(!m)d.w.h=s;
    else{for(p=22;!(m&(1u<<p));p--);e=p+874;m&=(1u<<p)-1;d.w.h=s|(e<<20);
        if(p>20)d.w.h|=m>>(p-20),d.w.l=m<<(52-p);else d.w.h|=m<<(20-p);}
    return d.d;
}

double __attribute__((pcs("aapcs"))) __aeabi_dadd(double a, double b) {
    return __aeabi_f2d(__aeabi_d2f(a)+__aeabi_d2f(b));
}

double __attribute__((pcs("aapcs"))) __aeabi_l2d(long long x) {
    return __aeabi_f2d(__aeabi_l2f(x));
}

double __attribute__((pcs("aapcs"))) __aeabi_ui2d(unsigned x) {
    return __aeabi_f2d(__aeabi_l2f(x));
}

double __attribute__((pcs("aapcs"))) __aeabi_i2d(int x) {
    return __aeabi_f2d(__aeabi_l2f(x));
}

long long __attribute__((pcs("aapcs")))  __aeabi_d2lz(double x) {
    return __aeabi_f2lz(__aeabi_d2f(x));
}

int __attribute__((pcs("aapcs"))) __aeabi_dcmpeq(double a, double b) {
    D64 x={.d=a},y={.d=b}; unsigned long long A=x.u,B=y.u,m=0xfffffffffffffULL;
    return !((((A>>52)&2047)==2047&&(A&m))||(((B>>52)&2047)==2047&&(B&m)))&&(A==B||!((A|B)<<1));
}

#endif
