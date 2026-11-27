#include <stdio.h>
#include <stdlib.h>

#define W 24
#define H 7

void msleep(unsigned);
int getchar_timeout(unsigned);

static char bg[H][W];

static void stars(void)
{
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W - 1; x++) bg[y][x] = bg[y][x + 1];
    bg[y][W - 1] = rand() & 0x700 ? ' ' : '.'; // 1/8
  }
}

int main(void)
{
  int y = H / 2, sx = -1, sy = 0;
  const char *shot = "";
  printf("\x1b[2J\x1b[?25l");
  for (;;) {
    int c = getchar_timeout(0);
    if (c == 'w' && y > 0) y--;
    if (c == 's' && y < H - 1) y++;
    if (c >= '1' && c <= '3') {
      shot = c == '1' ? "🔥" : c == '2' ? "⭐" : "💧";
      sx = 1; sy = y;
    }
    if (c == 'q') break;
    stars();
    printf("\x1b[H ROCKET  w/s:move  1:🔥 2:⭐ 3:💧  q:quit\x1b[K\n");
    for (int r = 0; r < H; r++) {
      for (int i = 0; i < W; i++) {
        if (r == y && i == 0) printf("🚀");
        else if (r == sy && i == sx) printf("%s", shot);
        else printf("%c ", bg[r][i] ? bg[r][i] : ' ');
      }
      printf("\x1b[K\n");
    }
    if (sx >= 0 && ++sx >= W) sx = -1;
    msleep(50);
  }
  printf("\x1b[?25h\n");
  return 0;
}