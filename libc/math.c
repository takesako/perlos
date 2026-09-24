#include "math.h"

typedef union {float f; unsigned u;} F32;

static float nan_(void) {return ((F32){.u=0x7fc00000}).f;}
static float inf_(void) {return ((F32){.u=0x7f800000}).f;}

int isnanf(float x) {F32 a={x};return (a.u&0x7fffffff)>0x7f800000;}
int isinff(float x) {F32 a={x};return (a.u&0x7fffffff)==0x7f800000;}
int isfinitef(float x) {F32 a={x};return (a.u&0x7f800000)!=0x7f800000;}
float fabsf(float x) {F32 a={x};a.u&=0x7fffffff;return a.f;}
float fminf(float a,float b) {return isnanf(a)?b:isnanf(b)?a:a<b?a:b;}
float fmaxf(float a,float b) {return isnanf(a)?b:isnanf(b)?a:a>b?a:b;}
float truncf(float x) {F32 a={x};int e=((a.u>>23)&255)-127;if(e<0){a.u&=0x80000000;return a.f;}if(e>=23)return x;a.u&=~((1u<<(23-e))-1);return a.f;}
float floorf(float x) {float y=truncf(x);return x<y?y-1:y;}
float ceilf(float x) {float y=truncf(x);return x>y?y+1:y;}
float roundf(float x) {return x<0?ceilf(x-.5f):floorf(x+.5f);}
float fmodf(float x,float y) {if(!y||isinff(x)||isnanf(x)||isnanf(y))return nan_();if(isinff(y))return x;return x-truncf(x/y)*y;}

float sqrtf(float x)
{
    float r;
    __asm__("vsqrt.f32 %0,%1":"=t"(r):"t"(x));
    return r;
}

static float angle(float x) {
    float p=M_PI_F,q=2*M_PI_F;
    x=fmodf(x,q);
    if(x>p)x-=q;
    if(x<-p)x+=q;
    return x;
}

float sinf(float x) {
    float y;x=angle(x);
    if(x>M_PI_F/2)x=M_PI_F-x;
    if(x<-M_PI_F/2)x=-M_PI_F-x;
    y=x*x;
    return x*(1-y/6+y*y/120-y*y*y/5040);
}

float cosf(float x) {
    float y,s=1;x=angle(x);
    if(x>M_PI_F/2)x=M_PI_F-x,s=-1;
    if(x<-M_PI_F/2)x=-M_PI_F-x,s=-1;
    y=x*x;
    return s*(1-y/2+y*y/24-y*y*y/720
             +y*y*y*y/40320-y*y*y*y*y/3628800);
}

float tanf(float x) {
    float y,s,c;
    x=fmodf(x,M_PI_F);
    if(x>M_PI_F/2)x-=M_PI_F;
    if(x<-M_PI_F/2)x+=M_PI_F;
    y=x*x;
    s=x*(1-y/6+y*y/120-y*y*y/5040);
    c=1-y/2+y*y/24-y*y*y/720+y*y*y*y/40320-y*y*y*y*y/3628800;
    return s/c;
}

static float _pow2(int n) {
    F32 v;
    if(n>127)return inf_();
    if(n<-149)return 0;
    v.u=n>=-126?(unsigned)(n+127)<<23:1u<<(n+149);
    return v.f;
}

float expf(float x) {
    float l=.693147181f,r,z,y;int n;
    if(x>88.72284f)return inf_();
    if(x<-103.9721f)return 0;
    n=(int)(x/l);r=x-n*l;z=r*r;
    y=1+r+z/2+z*r/6+z*z/24+z*z*r/120;
    return y*_pow2(n);
}

float logf(float x) {
    float l=.693147181f,z,y,q;int e;
    if(x<0)return nan_();
    if(!x)return-inf_();
    if(isinff(x))return x;
    x=frexpf(x,&e)*2;e--;
    z=(x-1)/(x+1);q=z*z;
    y=z+z*q/3+z*q*q/5+z*q*q*q/7+z*q*q*q*q/9;
    return 2*y+e*l;
}

float powf(float x,float y) {
    float r; if(!y) return 1;
    if(!x) return y>0?0:inf_();
    if(x<0) {
        if(truncf(y)!=y) return nan_();
        r=expf(y*logf(-x)); return fmodf(fabsf(y),2)==1?-r:r;
    }
    return expf(y*logf(x));
}

static float _atan(float x) {
    float s=x<0?-1:1,a=fabsf(x),z=a>1?1/a:a;
    float r=M_PI_F/4*z-z*(z-1)*(.2447f+.0663f*z);
    return s*(a>1?M_PI_F/2-r:r);
}

float atan2f(float y,float x) {if(x>0)return _atan(y/x);if(x<0)return _atan(y/x)+(y>=0?M_PI_F:-M_PI_F);return y>0?M_PI_F/2:y<0?-M_PI_F/2:0;}
float modff(float x,float *i) {*i=truncf(x);return isinff(x)?(x<0?-0.0f:0.0f):x-*i;}
float frexpf(float x,int *e) {int n=0;float a=fabsf(x);if(!x||!isfinitef(x)){*e=0;return x;}while(a<.5f)x*=2,a*=2,n--;while(a>=1)x*=.5f,a*=.5f,n++;*e=n;return x;}
