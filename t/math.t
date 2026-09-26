#include <stdio.h>
#include <math.h>

static int no,fail;
#define ok(c,n) do{ no++; if(c) printf("ok %d - %s\n",no,n); else{ printf("not ok %d - %s\n",no,n); fail++; } }while(0)
static int near(float a,float b,float e){ return fabsf(a-b)<=e; }

int main(void)
{
    float n=NAN,i=INFINITY;
    printf("1..24\n");

    ok(fabsf(-3.5f)==3.5f,"fabsf");
    ok(fminf(2,3)==2 && fmaxf(2,3)==3,"min max");
    ok(fminf(n,3)==3 && fmaxf(n,3)==3,"min max nan");

    ok(truncf(3.9f)==3 && truncf(-3.9f)==-3,"truncf");
    ok(floorf(3.9f)==3 && floorf(-3.1f)==-4,"floorf");
    ok(ceilf(3.1f)==4 && ceilf(-3.9f)==-3,"ceilf");
    ok(roundf(2.5f)==3 && roundf(-2.5f)==-3,"roundf");

    ok(near(fmodf(7,3),1,1e-6f),"fmodf");
    ok(isnan(fmodf(1,0)),"fmod zero");
    ok(fmodf(2,i)==2,"fmod infinity");

    ok(near(sqrtf(4),2,1e-5f),"sqrt exact");
    ok(near(sqrtf(2),1.41421356f,1e-4f),"sqrt two");
    ok(sqrtf(0)==0,"sqrt zero");
    ok(isnan(sqrtf(-1)),"sqrt negative");

    ok(near(sinf(0),0,1e-6f),"sin zero");
    ok(near(sinf(M_PI_F/2),1,0.001f),"sin pi/2");
    ok(near(cosf(0),1,1e-6f),"cos zero");
    ok(near(cosf(M_PI_F),-1,0.01f),"cos pi");
    ok(near(tanf(M_PI_F/4),1,0.02f),"tan pi/4");

    ok(near(expf(0),1,1e-6f),"exp zero");
    ok(near(logf(1),0,1e-6f),"log one");
    ok(near(logf(expf(1)),1,0.01f),"log exp");
    ok(near(powf(2,3),8,0.01f),"pow integer");
    ok(isnan(powf(-2,.5f)) && near(powf(-2,3),-8,0.02f),"pow negative");

    return fail!=0;
}
