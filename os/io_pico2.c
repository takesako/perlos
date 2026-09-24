#include <stdint.h>
#include "timer.h"

#define R(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define U(o) R(0x50110000u+(o))
#define D(o) R(0x50100000u+(o))
#define B(k) D(0x80+4*(k))
#define P(o) ((volatile uint8_t *)(uintptr_t)(0x50100000u+(o)))
#define S(o) R(0xd0000000u+(o))
#define N 64
#define STALL 0x800
#define STK 128

static volatile uint8_t rx[N],tx[N]; static volatile unsigned rr,rw,tr,tw,ready; static unsigned rc;
static uint32_t stk[STK] __attribute__((aligned(8)));
static uint8_t pid[6],line[7],reply[2],conf,dtr,notify,setaddr;
static unsigned busy,stage,left,limit,zlp,addr,txzlp,rxlen,rxpos;
static const volatile uint8_t *src; enum{IDLE,READ,LINE,SIN,SOUT};

static const uint8_t dev[]={18,1,0,2,2,2,1,64,9,0x12,1,0,0,1,0,1,0,1};
static const uint8_t cfg[]={9,2,67,0,2,1,0,0x80,50,9,4,0,0,1,2,2,1,0,5,0x24,0,0x10,1,5,0x24,1,0,1,4,0x24,2,2,5,0x24,6,0,1,7,5,0x81,3,16,0,16,9,4,1,0,2,10,0,0,0,7,5,2,2,64,0,0,7,5,0x82,2,64,0,0};
static const uint8_t lang[]={4,3,9,4},name[]={18,3,'P',0,'i',0,'c',0,'o',0,' ',0,'C',0,'D',0,'C',0},ss[]={0xa1,0x20,0,0,0,0,2,0,3,0};

static void mb(void){__asm__ volatile("dmb sy":::"memory");}

static uint32_t fg(void){while(!(S(0x50)&1))__asm__ volatile("wfe");return S(0x58);}
static void fp(uint32_t v){while(!(S(0x50)&2)){}S(0x54)=v;__asm__ volatile("sev");}
static void start(void(*f)(void)){uint32_t q[]={0,0,1,R(0xe000ed08),(uint32_t)(stk+STK),(uint32_t)(uintptr_t)f|1};for(unsigned i=0;i<6;){if(!q[i]){while(S(0x50)&1)(void)S(0x58);__asm__ volatile("sev");}fp(q[i]);i=fg()==q[i]?i+1:0;}}

static volatile uint8_t *buf(unsigned k){return P(k<2?0x100:0x100+64*k);}
static void clr(unsigned o,uint32_t b){U(o)=b;}
static void fire(unsigned k,unsigned n){unsigned v=n|(pid[k]?0x2000:0)|(k&1?0:0x8000),d=16;pid[k]^=1;busy|=1u<<k;B(k)=v;__asm__ volatile("dsb sy\n1:subs %0,%0,#1\nbne 1b":"+r"(d)::"cc","memory");B(k)=v|0x400;}
static void arm(unsigned k,const volatile uint8_t*p,unsigned n){if(!(k&1))for(unsigned i=0;i<n;i++)buf(k)[i]=p[i];fire(k,n);}
static void cancel(unsigned m){U(0x60)=m;while((U(0x64)&m)!=m){}for(unsigned k=0;k<6;k++)if(m&(1u<<k))B(k)=0;clr(0x64,m);U(0x60)=0;clr(0x58,m);busy&=~m;}
static void stall(void){stage=IDLE;U(0x68)=3;B(0)=B(1)=STALL;}
static void ack(void){stage=SIN;arm(0,0,0);}
static void next(void){if(left||zlp){unsigned n=left>64?64:left;if(!n)zlp=0;arm(0,src,n);if(n)src+=n;left-=n;}else{stage=SOUT;arm(1,0,0);}}
static void send(const uint8_t*p,unsigned n){src=p;left=n<limit?n:limit;zlp=n<limit&&!(n&63);stage=READ;next();}
static void eps(unsigned on){for(unsigned k=2;k<6;k++){B(k)=D(4*k)=0;pid[k]=0;}busy&=3;rxlen=rxpos=txzlp=0;notify=on;if(on){D(8)=0xac000180;D(16)=0xa8000200;D(20)=0xa8000240;arm(5,0,64);}}
static void ireset(unsigned x){cancel(x?0x30:4);if(x){pid[4]=pid[5]=0;rxlen=rxpos=txzlp=0;arm(5,0,64);}else{pid[2]=0;notify=1;}}
static int ep(unsigned a){return a==0?1:a==0x80?0:a==0x81?2:a==0x82?4:a==2?5:-1;}

static void setup(void){
 unsigned a=D(0),b=D(4),v=a>>16,ix=b&65535,req=a&65535;int k=ep(ix);
 clr(0x50,1u<<17);pid[0]=pid[1]=1;busy&=~3u;stage=IDLE;U(0x68)=0;limit=b>>16;reply[0]=reply[1]=0;
 switch(req){
 case 0x0680:if(v==0x100){send(dev,sizeof dev);return;}if(v==0x200){send(cfg,sizeof cfg);return;}if(v==0x300){send(lang,sizeof lang);return;}if(v==0x301){send(name,sizeof name);return;}break;
 case 0x0500:if(v<128&&!ix&&!limit){addr=v;setaddr=1;ack();return;}break;
 case 0x0900:if(v<2&&!ix&&!limit){conf=v;dtr=0;eps(v);ack();return;}break;
 case 0x0880:if(!v&&!ix&&limit==1){send(&conf,1);return;}break;
 case 0x0080:if(!v&&!ix&&limit==2){send(reply,2);return;}break;
 case 0x0081:if(conf&&!v&&ix<2&&limit==2){send(reply,2);return;}break;
 case 0x0082:if(!v&&limit==2&&k>=0&&(k<2||conf)){reply[0]=!!(B(k)&STALL);send(reply,2);return;}break;
 case 0x0a81:if(conf&&!v&&ix<2&&limit==1){send(reply,1);return;}break;
 case 0x0b01:if(conf&&!v&&ix<2&&!limit){ireset(ix);ack();return;}break;
 case 0x0102:case 0x0302:if(conf&&!v&&!limit&&k>=2){cancel(1u<<k);pid[k]=0;if(k==4)txzlp=0;if(k==5)rxlen=rxpos=0;if(req==0x0302)B(k)=STALL;else if(k==5)arm(5,0,64);else if(k==2)notify=1;ack();return;}break;
 case 0x21a1:if(conf&&!v&&!ix){send(line,7);return;}break;
 case 0x2021:if(conf&&!v&&!ix&&limit==7){stage=LINE;arm(1,0,7);return;}break;
 case 0x2221:if(conf&&!ix&&!limit&&v<4){dtr=v&1;notify=1;ack();return;}break;
 }
 stall();
}

static void task(void){
 unsigned st=U(0x50);
 if(st&(1u<<19)){clr(0x50,1u<<19);eps(0);B(0)=B(1)=0;clr(0x58,3);U(0)=U(0x68)=0;pid[0]=pid[1]=0;busy=conf=dtr=stage=rxlen=rxpos=setaddr=addr=0;return;}
 unsigned ns=st&(1u<<17),s=U(0x58);clr(0x58,s);
 if(ns){s&=~3u;busy&=~3u;setup();}else busy&=~s;
 if(s&1){if(setaddr){U(0)=addr;__asm__ volatile("dsb sy":::"memory");setaddr=0;stage=IDLE;}else if(stage==READ)next();else stage=IDLE;}
 if(s&2){if(stage==LINE&&(B(1)&1023)==7){for(unsigned i=0;i<7;i++)line[i]=buf(1)[i];ack();}else if(stage==SOUT)stage=IDLE;else stall();}
 if(s&32){rxlen=B(5)&1023;rxpos=0;if(!rxlen)arm(5,0,64);}
 if(rxpos<rxlen){unsigned w=rw,n=rxlen-rxpos,f=N-(w-rr);if(n>f)n=f;if(n){for(unsigned i=0;i<n;i++)rx[(w+i)&(N-1)]=buf(5)[rxpos+i];mb();rw=w+n;rxpos+=n;}}
 if(rxpos==rxlen&&rxlen){rxlen=rxpos=0;if(!(B(5)&STALL))arm(5,0,64);}
 if(!conf)return;
 if(notify&&!(busy&4)&&!(B(2)&STALL)){arm(2,ss,sizeof ss);notify=0;}
 if((busy&16)||(B(4)&STALL))return;
 if(txzlp){arm(4,0,0);txzlp=0;return;}
 if(tr!=tw){unsigned r=tr,n=tw-r;if(n>64)n=64;mb();for(unsigned i=0;i<n;i++)buf(4)[i]=tx[(r+i)&(N-1)];mb();tr=r+n;fire(4,n);if(n==64&&tr==tw)txzlp=1;}
}

static void pll(uintptr_t b,unsigned fb,unsigned post){R(b)=1;R(b+8)=fb;R(b+4)=12;while(!(R(b)&0x80000000u)){}R(b+12)=post;R(b+4)=4;}
static void delay(void){for(volatile unsigned n=6000000;n--;);}

static void hw(void){
 R(0x40022000)=1u<<28;R(0x40010084)=0;
 R(0x4004800c)=47;R(0x40048000)=0xfabaa0;while(!(R(0x40048004)&0x80000000u)){}
 R(0x40010030)=2;while(!(R(0x40010038)&4)){}R(0x4001303c)=1;while(!(R(0x40010044)&1)){}
 R(0x40010034)=R(0x40010040)=0x10000;R(0x40010060)=0;while(R(0x40010060)&0x10000000u){}
 R(0x40022000)=0xc000;R(0x40023000)=0xc000;while((R(0x40020008)&0xc000)!=0xc000){}
 pll(0x40050000,125,0x52000); // 12*125/5/2 = 150 MHz
 pll(0x40058000,100,0x55000); // 12*100/5/5 = 48 MHz
 unsigned q=R(0x400d000c);if((q&255)&&(q&255)<4){R(0x400d000c)=(q&~255u)|4;__asm__ volatile("dsb sy":::"memory");(void)R(0x14000000);}
 R(0x4001003c)=0;R(0x4001003c)=1;while(!(R(0x40010044)&2)){}
 R(0x40010064)=0x10000;R(0x40010060)=0x800;while(!(R(0x40010060)&0x10000000u)){}
 R(0x40023000)=1u<<28;while(!(R(0x40020008)&(1u<<28))){}
 for(unsigned i=0;i<4096;i+=4)D(i)=0;line[1]=0xc2;line[2]=1;line[6]=8;
 U(0x74)=9;U(0x78)=12;U(0x40)=1;U(0x4c)=0x20000000;delay();U(0x4c)=0x20010000;
}

static void core1(void){hw();mb();ready=1;__asm__ volatile("sev");for(;;)task();}

void _io_init(void){start(core1);while(!ready)__asm__ volatile("wfe");mb();}

__attribute__((noreturn))
void _io_exit(int code __attribute__((unused))){for(;;)__asm volatile("wfi");}

int putchar(int c){
  while(tw-tr==N)__asm volatile("wfe");
  tx[tw&(N-1)]=c;mb();tw++;return c;
}

int getchar(void){
  while(rc==rw)__asm volatile("wfe");
  mb();int c=rx[rc++&(N-1)];mb();rr=rc;return c;
}

int getchar_timeout(unsigned ms){
  unsigned t=gettick();
  do{
    if(rc!=rw){mb();int c=rx[rc++&(N-1)];mb();rr=rc;return c;}
    __asm volatile("wfe");
  }while(gettick()-t<ms);
  return -1;
}

int console_write(const void *data,unsigned size){
  const unsigned char *p=data;unsigned n=size,w,k,i;
  while(n){
    while(tw-tr==N)__asm volatile("wfe");
    k=N-(tw-tr);if(k>n)k=n;w=tw;
    for(i=0;i<k;i++)tx[(w+i)&(N-1)]=p[i];
    mb();tw=w+k;p+=k;n-=k;
  }
  return size;
}
