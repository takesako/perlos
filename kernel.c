#include "io.h"

extern int main(void);

#ifdef PICO2
#define CPU_HZ 150000000u
#else
#define CPU_HZ 20000000u
#endif

#define R(a) (*(volatile unsigned *)(a))

void kernel_main(void)
{
  R(0xe000e014) = CPU_HZ / 1000 - 1;
  R(0xe000e018) = 0;
  R(0xe000e010) = 7;
  __asm volatile("cpsie i");
  _io_init();
  int code = main();
  _io_exit(code);
}
