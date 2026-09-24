#include <stdio.h>

int main(void)
{
  char str[] = "Hello Pico2 OS!\n";
  for (char *s = str; *s; s++) putchar(*s);
  for(;;) {
    int c = getchar();
    if(c == -1) return 1;
    if(c == '\r') c = '\n'; // for Windows
    putchar(c);
  }
}