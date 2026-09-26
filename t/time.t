#include <stdio.h>
#include <time.h>
#include <sys/times.h>
#include <errno.h>
#include <timer.h>
static int no, fail;
#define ok(c,n) do { no++; if(c) printf("ok %d - %s\n",no,n); \
else { printf("not ok %d - %s\n",no,n); fail++; } } while(0)
int main(void)
{
    struct tms a,b;
    puts("1..6");
    clock_t t=times(&a);
    ok(t==a.tms_utime && !a.tms_stime && !a.tms_cutime && !a.tms_cstime,"single-task times fields");
    ok(CLK_TCK==1000 && CLOCKS_PER_SEC==1000,"tick scaling");
    msleep(25); times(&b);
    ok(b.tms_utime-a.tms_utime>=25,"time advances in milliseconds");
    ok(clock()>=b.tms_utime,"clock shares times timebase");
    time_t n=time(NULL);
    ok(n<=clock()/CLOCKS_PER_SEC && clock()/CLOCKS_PER_SEC-n<=1,"time in seconds");
    ok(times(NULL)==(clock_t)-1 && errno==EINVAL,"null times rejected");
    return fail!=0;
}
