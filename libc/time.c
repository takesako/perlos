#include <time.h>
#include <limits.h>
#include <errno.h>
#include "timer.h"

time_t time(time_t *p) // No RTC: epoch 0 is boot
{
    static unsigned last; static unsigned long long ticks;
    unsigned now = gettick(); ticks += (unsigned)(now-last); last = now;
    time_t t = (time_t)(ticks/1000); if (p) *p = t; return t;
}

clock_t clock(void) { return (clock_t)gettick(); }

static long long days(long long y, int m, int d)
{
    y -= m <= 2;
    long long era = (y >= 0 ? y : y-399)/400;
    int yo = (int)(y-era*400);
    int doy = (153*(m+(m>2?-3:9))+2)/5+d-1;
    return era*146097+yo*365+yo/4-yo/100+doy-719468;
}

struct tm *gmtime(const time_t *p)
{
    static struct tm t;
    if (!p) { errno = EINVAL; return 0; }
    long long d = *p/86400, rem = *p%86400;
    if (rem < 0) rem += 86400, d--;
    long long z=d+719468, era=(z>=0?z:z-146096)/146097;
    int doe=(int)(z-era*146097);
    int yo=(doe-doe/1460+doe/36524-doe/146096)/365;
    long long y=yo+era*400;
    int doy=doe-(365*yo+yo/4-yo/100), mp=(5*doy+2)/153;
    int day=doy-(153*mp+2)/5+1, month=mp+(mp<10?3:-9);
    y += month <= 2;
    if (y-1900 < INT_MIN || y-1900 > INT_MAX) { errno = EOVERFLOW; return 0; }
    t = (struct tm){(int)(rem%60), (int)(rem/60%60), (int)(rem/3600),
        day, month-1, (int)(y-1900), (int)((d+4)%7),
        (int)(d-days(y,1,1)), 0};
    if (t.tm_wday < 0) t.tm_wday += 7;
    return &t;
}

struct tm *localtime(const time_t *p) { return gmtime(p); } // UTC only.

time_t mktime(struct tm *t)
{
    if (!t) { errno=EINVAL; return -1; }
    long long y=(long long)t->tm_year+1900, m=t->tm_mon;
    y+=m/12; m%=12; if(m<0)m+=12,y--;
    time_t result=(days(y,(int)m+1,1)+(long long)t->tm_mday-1)*86400+
        (long long)t->tm_hour*3600+(long long)t->tm_min*60+t->tm_sec;
    struct tm *n=gmtime(&result); if(!n) return -1;
    *t=*n; return result;
}
