#include "timer.h"

#define UART(o) (*(volatile unsigned *)(0x50200000u+(o)))

void _io_init(void){
  UART(0x10) = 16;
  UART(8) = 3;
}

__attribute__((noreturn))
void _io_exit(int code)
{
  unsigned int a[2] = {0x20026u, (unsigned int)code};
  register unsigned int r0 asm("r0") = 0x20;
  register unsigned int *r1 asm("r1") = a;
  __asm volatile(
    "bkpt #0xab"
    :
    : "r"(r0), "r"(r1)
    : "memory"
  );
  for (;;) __asm volatile("wfi");
}

int putchar(int c){
  while(UART(4)&1){}
  UART(0)=c;return c;
}

int getchar(void){
  while(!(UART(4)&2))__asm volatile("wfi");
  return UART(0)&255;
}

int getchar_timeout(unsigned ms){
  unsigned t=gettick();
  do{
    if(UART(4)&2)return UART(0)&255;
    __asm volatile("wfi");
  }while(gettick()-t<ms);
  return -1;
}

int console_write(const void *p,unsigned n){
  const unsigned char *s=p;unsigned z=n;
  while(n--){while(UART(4)&1){}UART(0)=*s++;}
  return z;
}
