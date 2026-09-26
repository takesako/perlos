#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#ifdef ROMFS
#define _NFILE 8
extern const unsigned char *_romfs_find(const char *,size_t *);
static FILE _file[_NFILE];
#endif

FILE _stdin={.push=EOF},_stdout={.push=EOF},_stderr={.push=EOF};

static int rmode(const char *m){return m&&*m=='r'&&(!m[1]||(m[1]=='b'&&!m[2]));}

FILE *fopen(const char *n,const char *m){
#ifdef ROMFS
 size_t z;const unsigned char *p;
 if(!rmode(m)||!(p=_romfs_find(n,&z)))return 0;
 for(int i=0;i<_NFILE;i++)if(!_file[i].p)return _file[i]=(FILE){p,z,0,EOF},&_file[i];
#else
 (void)n;(void)m;
#endif
 return 0;
}

FILE *freopen(const char *n,const char *m,FILE *f){
#ifdef ROMFS
 size_t z;const unsigned char *p;
 if(!f||!rmode(m)||!(p=_romfs_find(n,&z)))return 0;
 *f=(FILE){p,z,0,EOF};return f;
#else
 (void)n;(void)m;(void)f;return 0;
#endif
}

FILE *fdopen(int fd,const char *m){(void)m;return fd==0?stdin:fd==1?stdout:fd==2?stderr:0;}
FILE *tmpfile(void){errno=EROFS;return 0;}

int fclose(FILE *f){
#ifdef ROMFS
 if(f&&f!=stdin&&f!=stdout&&f!=stderr)*f=(FILE){0,0,0,EOF};
#else
 (void)f;
#endif
 return 0;
}

int fgetc(FILE *f){
 if(!f)return EOF;
 if(f->push!=EOF){int c=f->push;f->push=EOF;return c;}
 if(f==stdin)return getchar();
#ifdef ROMFS
 if(f!=stdout&&f!=stderr&&f->pos<f->size)return f->p[f->pos++];
#endif
 return EOF;
}

int ungetc(int c,FILE *f){
 if(!f||f==stdout||f==stderr||c==EOF||f->push!=EOF)return EOF;
 return f->push=(unsigned char)c;
}

size_t fread(void *p,size_t s,size_t n,FILE *f){
 unsigned char *q=p;size_t i=0,z=s*n;int c;
 if(!s)return 0;
 while(i<z&&(c=fgetc(f))!=EOF)q[i++]=c;
 return i/s;
}

size_t fwrite(const void *p,size_t s,size_t n,FILE *f){
 size_t z=s*n;
 if(!s||!n||(f!=stdout&&f!=stderr))return 0;
 int r=console_write(p,(unsigned)z);
 return r>0?(size_t)r/s:0;
}

int fputc(int c,FILE *f){return f==stdout||f==stderr?putchar(c):EOF;}

char *fgets(char *s,int n,FILE *f){
 int i=0,c;if(!s||n<=0)return 0;
 while(i+1<n&&(c=fgetc(f))!=EOF){s[i++]=c;if(c=='\n')break;}
 if(!i)return 0;s[i]=0;return s;
}

int fputs(const char *s,FILE *f){while(*s)if(fputc(*s++,f)==EOF)return EOF;return 0;}
int puts(const char *s){return fputs(s,stdout)==EOF||fputc('\n',stdout)==EOF?EOF:0;}

int fseek(FILE *f,long off,int w){
#ifdef ROMFS
 if(!f||f==stdin||f==stdout||f==stderr)return-1;
 long cur=(long)f->pos-(f->push!=EOF);
 long p=w==SEEK_SET?off:w==SEEK_CUR?cur+off:w==SEEK_END?(long)f->size+off:-1;
 if(p<0)return-1;f->pos=p;f->push=EOF;return 0;
#else
 (void)f;(void)off;(void)w;return-1;
#endif
}

long ftell(FILE *f){
#ifdef ROMFS
 if(f&&f!=stdin&&f!=stdout&&f!=stderr)return(long)f->pos-(f->push!=EOF);
#else
 (void)f;
#endif
 return-1;
}

void rewind(FILE *f){(void)fseek(f,0,SEEK_SET);}
int fgetpos(FILE *f,fpos_t *p){long n;if(!p||(n=ftell(f))<0)return-1;*p=n;return 0;}
int fsetpos(FILE *f,const fpos_t *p){return p?fseek(f,*p,SEEK_SET):-1;}

int feof(FILE *f){
#ifdef ROMFS
 return f&&f!=stdin&&f!=stdout&&f!=stderr&&f->push==EOF&&f->pos>=f->size;
#else
 (void)f;return 0;
#endif
}

int ferror(FILE *f){(void)f;return 0;}
void clearerr(FILE *f){(void)f;}
int fflush(FILE *f){(void)f;return 0;}
void setbuf(FILE *f,char *b){(void)f;(void)b;}
int setvbuf(FILE *f,char *b,int m,size_t n){(void)f;(void)b;(void)m;(void)n;return 0;}
int fileno(FILE *f){return f==stdin?0:f==stdout?1:f==stderr?2:-1;}

int remove(const char *p){(void)p;errno=EROFS;return-1;}
int rename(const char *a,const char *b){(void)a;(void)b;errno=EROFS;return-1;}

void perror(const char *s){
 int e=errno;
 if(s&&*s)fputs(s,stderr),fputs(": ",stderr);
 fputs(strerror(e),stderr);fputc('\n',stderr);
}

/* printf */

typedef struct{FILE *f;char *s;size_t n,z;int e;}OUT;

static void put(OUT *o,int c){
 if(o->f){if(fputc(c,o->f)==EOF)o->e=1;}
 else if(o->z&&o->n<o->z-1)o->s[o->n]=c;
 o->n++;
}

static unsigned f32(double x){union{float f;unsigned u;}v={(float)x};return v.u;}

static void fmul(unsigned char *d,int *n,int m){
 int i,c=0,x;
 for(i=0;i<*n;i++)x=d[i]*m+c,d[i]=x%10,c=x/10;
 while(c)d[(*n)++]=c%10,c/=10;
}

static int fdig(unsigned u,unsigned char *d,int *dp){
 int e=u>>23&255,n=0,k,i;unsigned m=u&0x7fffff;unsigned char c;
 if(!e&&!m)return d[0]=0,*dp=1,1;
 if(e)m|=0x800000,e-=150;else e=-149;
 do d[n++]=m%10,m/=10;while(m);
 k=e<0?-e:0;
 while(e>0)fmul(d,&n,2),e--;
 while(e<0)fmul(d,&n,5),e++;
 *dp=n-k;
 for(i=0;i<n/2;i++)c=d[i],d[i]=d[n-1-i],d[n-1-i]=c;
 return n;
}

static void rnd(unsigned char *d,int *n,int *dp,int k){
 int i;
 if(k<0){d[0]=0;*n=*dp=1;return;}
 if(!k){if(d[0]>=5)d[0]=1,*n=1,(*dp)++;else d[0]=0,*n=*dp=1;return;}
 if(k<*n&&d[k]>=5){
  for(i=k;i&&d[i-1]==9;)d[--i]=0;
  if(i)d[i-1]++;
  else{d[0]=1;for(i=1;i<k;i++)d[i]=0;(*dp)++;}
 }
 if(k<*n)*n=k;
}

static int ftoa(char *s,unsigned u,int p,int t){
 unsigned char d[112];char *q=s,*dot=0,*x;int n,dp,i,e,trim=0;
 if(u>>31)*q++='-';
 if((u&0x7f800000)==0x7f800000){
  x=(u&0x7fffff)?"nan":"inf";while(*x)*q++=*x++;*q=0;return q-s;
 }
 n=fdig(u&0x7fffffff,d,&dp);if(p<0)p=6;if(p>63)p=63;
 if(t=='g'){
  if(!p)p=1;rnd(d,&n,&dp,p);e=dp-1;trim=1;
  if(e<-4||e>=p)t='e',p--;else t='f',p-=dp;
  if(p<0)p=0;
 }else if(t=='e')rnd(d,&n,&dp,p+1);else rnd(d,&n,&dp,dp+p);
 if(t=='f'){
  if(dp<=0)*q++='0';else for(i=0;i<dp;i++)*q++=(i<n?d[i]:0)+'0';
  if(p){dot=q;*q++='.';for(i=0;i<p;i++){int j=dp+i;*q++=(j>=0&&j<n?d[j]:0)+'0';}}
 }else{
  *q++=d[0]+'0';
  if(p){dot=q;*q++='.';for(i=1;i<=p;i++)*q++=(i<n?d[i]:0)+'0';}
 }
 if(trim&&dot){while(q>dot+1&&q[-1]=='0')q--;if(q==dot+1)q=dot;}
 if(t=='e'){e=dp-1;*q++='e';*q++=e<0?'-':'+';if(e<0)e=-e;*q++='0'+e/10;*q++='0'+e%10;}
 *q=0;return q-s;
}

static int vfmt(OUT *o,const char *f,__builtin_va_list a){
#define P(c) put(o,c)
 while(*f){
  if(*f!='%'){P(*f++);continue;}
  if(!*++f){P('%');break;}
  char pad=*f=='0'?*f++:' ';int w=0,p=-1,l=0,z=0;
  if(*f=='*')w=__builtin_va_arg(a,int),f++;
  else while(*f>='0'&&*f<='9')w=w*10+*f++-'0';
  if(*f=='.'){
   f++;p=0;
   if(*f=='*')p=__builtin_va_arg(a,int),f++;
   else while(*f>='0'&&*f<='9')p=p*10+*f++-'0';
  }
  if(*f=='l')l=1,f++;
  else if(*f=='z')z=1,f++;
  if(*f=='s'){
   char *s=__builtin_va_arg(a,char *);int n=0;if(!s)s="(null)";
   while(s[n])n++;while(n++<w)P(pad);while(*s)P(*s++);
  }else if(*f=='c')P(__builtin_va_arg(a,int));
  else if(*f=='d'||*f=='u'||*f=='x'||*f=='X'){
   unsigned long x;char b[24];int i=0,m=0,base=(*f=='x'||*f=='X')?16:10;
   const char *d=*f=='X'?"0123456789ABCDEF":"0123456789abcdef";
   if(*f=='d'){
    long v;
    if(z)v=(long)__builtin_va_arg(a,ssize_t);
    else v=l?__builtin_va_arg(a,long):__builtin_va_arg(a,int);
    m=v<0;x=m?0ul-(unsigned long)v:(unsigned long)v;
   }else{
    if(z)x=(unsigned long)__builtin_va_arg(a,size_t);
    else x=l?__builtin_va_arg(a,unsigned long):__builtin_va_arg(a,unsigned);
   }
   do b[i++]=d[x%base],x/=base;while(x);
   int n=i+m;if(m&&pad=='0')P('-'),m=0;while(n++<w)P(pad);
   if(m)P('-');while(i)P(b[--i]);
  }else if(*f=='f'||*f=='e'||*f=='g'){
   char b[128],*s=b;int n=ftoa(b,f32(__builtin_va_arg(a,double)),p,*f);
   if(pad=='0'&&*s=='-'&&n<w)P(*s++),n--,w--;
   while(n<w)P(pad),w--;while(*s)P(*s++);
  }else{P('%');if(*f!='%')P(*f);}
  if(*f)f++;
 }
 if(!o->f&&o->z)o->s[(o->n<o->z)?(o->n):(o->z-1)]=0;
#undef P
 return o->e?EOF:(int)o->n;
}

int vfprintf(FILE *fp,const char *f,__builtin_va_list a){OUT o={fp,0,0,0,0};return vfmt(&o,f,a);}
int vsnprintf(char *s,size_t z,const char *f,__builtin_va_list a){OUT o={0,s,0,z,0};return vfmt(&o,f,a);}
int vsprintf(char *s,const char *f,__builtin_va_list a){return vsnprintf(s,(size_t)-1,f,a);}
int vprintf(const char *f,__builtin_va_list a){return vfprintf(stdout,f,a);}

int printf(const char *f,...){
 __builtin_va_list a;__builtin_va_start(a,f);int n=vfprintf(stdout,f,a);__builtin_va_end(a);return n;
}
int fprintf(FILE *fp,const char *f,...){
 __builtin_va_list a;__builtin_va_start(a,f);int n=vfprintf(fp,f,a);__builtin_va_end(a);return n;
}
int snprintf(char *s,size_t z,const char *f,...){
 __builtin_va_list a;__builtin_va_start(a,f);int n=vsnprintf(s,z,f,a);__builtin_va_end(a);return n;
}
int sprintf(char *s,const char *f,...){
 __builtin_va_list a;__builtin_va_start(a,f);int n=vsprintf(s,f,a);__builtin_va_end(a);return n;
}
