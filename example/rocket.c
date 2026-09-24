#include <stdio.h>

#define W 24
#define H 7

void msleep(unsigned);
int getchar_timeout(unsigned);
int rand(void);

static char bg[H][W];

static void stars(void){
  for(int y=0;y<H;y++){
    for(int x=0;x<W-1;x++)bg[y][x]=bg[y][x+1];
    bg[y][W-1]=rand()%8?' ':'.';
  }
}

int main(void){
  int y=H/2,x=-1;
  const char *shot=0;

  printf("\x1b[2J\x1b[?25l");
  for(;;){
    int c=getchar_timeout(0);

    if(c=='w'&&y>0)y--;
    if(c=='s'&&y<H-1)y++;
    if(c=='1'){shot="🔥";x=0;}
    if(c=='2'){shot="⭐";x=0;}
    if(c=='3'){shot="💧";x=0;}
    if(c=='q')break;

    stars();
    printf("\x1b[H");

    for(int r=0;r<H;r++){
      printf("%s",r==y?"🚀":"  ");
      for(int i=0;i<W;i++)
        if(r==y&&i==x)printf("%s",shot);
        else printf("%c ",bg[r][i]);
      printf("\x1b[K\n");
    }

    if(x>=0&&++x>=W)x=-1;
    msleep(50);
  }

  printf("\x1b[?25h\n");
  return 0;
}