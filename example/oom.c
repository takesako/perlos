#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include "timer.h"

int main(void) {
  const unsigned char colors[] = {
    0x00, // Black      Zero
    0x01, // Green      Control
    0x09, // Light blue TAB
    0x0a, // Blue       LF
    0x20, // Pink       Space
    0x41, // Red        ASCII 'A'
    0x80, // Cyan       Binary
    0xff, // White      0xFF
  };
  const unsigned ncolors = sizeof(colors) / sizeof(colors[0]);

  unsigned blocks = 0;
  for (;;) {
    unsigned char *p = malloc(1024);
    if (p == NULL) {
      printf("Out of memory: malloc returned NULL\r\n");
      printf("%u blocks = %u KiB\r\n", blocks, blocks);
      heap_dump("out of memory");
      return 0;
    }
    memset(p, colors[blocks % ncolors], 1024);
    blocks++;
    msleep(20);
  }
}
