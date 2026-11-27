#include "io.h"

#define R(a) (*(volatile unsigned *)(a))

extern int main(void);

void kernel_main(void)
{
  R(0xe000e014) = CPU_MHZ * 1000 - 1;
  R(0xe000e018) = 0;
  R(0xe000e010) = 7;
  __asm volatile("cpsie i");
  _io_init();
  _io_exit(main());
}