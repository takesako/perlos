#include <stdio.h>

int main(void)
{
  char str[] = "Hello Pico2 OS!\n";
  for (char *s = str; *s; s++) putchar(*s);
  console_write((void *)str, (unsigned)sizeof(str)-1);
  for(;;) {
    int c = getchar_timeout(1000);
    if(c == -1) {
      putchar('.');
      continue;
    }
    if(c == '\r') {
      putchar('\n'); // for Windows
      putchar(getchar()); // blocking
    } else {
      putchar(c);
    }
  }
}