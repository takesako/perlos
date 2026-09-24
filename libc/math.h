#ifndef _MATH_H
#define _MATH_H

#define M_PI_F 3.141592654f
#define INFINITY (__builtin_inff())
#define HUGE_VAL (__builtin_huge_val())
#define HUGE_VALF (__builtin_huge_valf())
#define HUGE_VALL (__builtin_huge_vall())
#define NAN (__builtin_nanf(""))

float fabsf(float);
float fminf(float,float);
float fmaxf(float,float);
float truncf(float);
float floorf(float);
float ceilf(float);
float roundf(float);
float fmodf(float,float);
float sqrtf(float);
float sinf(float);
float cosf(float);
float tanf(float);
float expf(float);
float logf(float);
float powf(float,float);
float atan2f(float,float);
float modff(float,float *);
float frexpf(float,int *);

int isnanf(float);
int isinff(float);
int isfinitef(float);

#define isnan(x) isnanf(x)
#define isinf(x) isinff(x)
#define isfinite(x) isfinitef(x)

#endif
