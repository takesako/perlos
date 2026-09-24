#include <romfs.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include "io.h"

#define NFD 32
enum{FD_IN=1,FD_OUT,FD_ERR,FD_ROM};

static const unsigned char *base;
static const struct romfs_header *hdr;
static char cwd[PATH_MAX];

struct desc{const unsigned char *p;size_t size,pos;unsigned refs;int kind;};
static struct desc obj[NFD]={{0,0,0,1,FD_IN},{0,0,0,1,FD_OUT},{0,0,0,1,FD_ERR}};
static struct desc *fds[NFD]={obj,obj+1,obj+2};
static char *empty_env[]={0};
char **environ=empty_env;

static int fail(int e){errno=e;return-1;}
static const unsigned char *a4(const unsigned char *p){return base+(((size_t)(p-base)+3)&~(size_t)3);}
static const unsigned char *data(const struct romfs_entry *e){return a4((const unsigned char *)(e+1)+e->name_len+1);}
static const struct romfs_entry *next(const struct romfs_entry *e){return(const void *)a4(data(e)+e->size+1);}

int romfs_init(const void *p){
 const unsigned char *b=p,*q,*end;const struct romfs_header *h=p;const struct romfs_entry *e;
 base=0;hdr=0;cwd[0]=0;if(!p)return-1;if(memcmp(h->magic,ROMFS_MAGIC,4)||h->total_size<sizeof *h)return-1;
 base=b;end=b+h->total_size;q=b+sizeof *h;
 while(q<end){
  if((size_t)(end-q)<sizeof *e)goto bad;e=(const void *)q;q=(const void *)(e+1);
  if(!e->name_len||e->name_len>=(size_t)(end-q)||q[e->name_len])goto bad;q+=e->name_len+1;q=a4(q);if(q>end)goto bad;
  if(e->size>=(size_t)(end-q)||q[e->size])goto bad;q+=e->size+1;q=a4(q);if(q>end)goto bad;
 }
 if(q!=end)goto bad;hdr=h;return 0;
bad:base=0;return-1;
}

static int path(const char *s,char out[PATH_MAX]){
 size_t n;if(!s)return fail(EFAULT);if(!*s)return fail(ENOENT);
 n=*s=='/'?0:strlen(cwd);memcpy(out,cwd,n);out[n]=0;
 while(*s){
  while(*s=='/')s++;const char *p=s;while(*s&&*s!='/')s++;size_t k=s-p;
  if(!k||(k==1&&*p=='.'))continue;
  if(k==2&&p[0]=='.'&&p[1]=='.'){while(n&&out[n-1]!='/')n--;if(n)n--;out[n]=0;continue;}
  if(n+!!n+k>=PATH_MAX)return fail(ENAMETOOLONG);
  if(n)out[n++]='/';memcpy(out+n,p,k);n+=k;out[n]=0;
 }
 return 0;
}

static const unsigned char *find0(const char *name,size_t *size){
 size_t n=strlen(name);const unsigned char *end=base+hdr->total_size;const struct romfs_entry *e=(const void *)(base+sizeof *hdr);
 if(size)*size=0;while((const unsigned char *)e<end){if(e->name_len==n&&!memcmp(name,e->name,n)){if(size)*size=e->size;return data(e);}e=next(e);}return 0;
}

static int isdir0(const char *name){
 size_t n=strlen(name);const unsigned char *end=base+hdr->total_size;const struct romfs_entry *e=(const void *)(base+sizeof *hdr);
 if(!n)return 1;while((const unsigned char *)e<end){if(e->name_len>n&&e->name[n]=='/'&&!memcmp(e->name,name,n))return 1;e=next(e);}return 0;
}

const unsigned char *_romfs_find(const char *name,size_t *size){char p[PATH_MAX];if(size)*size=0;if(!hdr||path(name,p))return 0;return find0(p,size);}
const unsigned char *romfs_data_for_compile(const char *n,size_t *z){return _romfs_find(n,z);}
int romfs_isdir(const char *name){char p[PATH_MAX];if(!hdr)return 0;if(name&&!*name)return 1;return !path(name,p)&&isdir0(p);}

#define A4(n) (((n)+3)&~3u)

void romfs_each(void (*fn)(const char *,void *),void *arg)
{
    const struct romfs_entry *e;
    const unsigned char *end;
    if(!base||!hdr)return;
    e=(const void *)(base+sizeof *hdr);
    end=base+hdr->total_size;
    while((const unsigned char *)e<end){
        fn(e->name,arg);
        e=next(e);
    }
}

static struct desc *get(int fd){if(fd<0||fd>=NFD||!fds[fd]){errno=EBADF;return 0;}return fds[fd];}
static int slot(void){for(int i=3;i<NFD;i++)if(!fds[i])return i;return fail(EMFILE);}

int open(const char *name,int flags,...){
 char p[PATH_MAX];size_t n;const unsigned char *d;if(flags)return fail(EROFS);if(path(name,p))return-1;
 if(isdir0(p))return fail(EISDIR);if(!(d=find0(p,&n)))return fail(ENOENT);if(n>LONG_MAX)return fail(EOVERFLOW);
 int fd=slot();if(fd<0)return-1;for(int i=3;i<NFD;i++)if(!obj[i].refs){obj[i]=(struct desc){d,n,0,1,FD_ROM};fds[fd]=obj+i;return fd;}return fail(ENFILE);
}

int close(int fd){struct desc *d=get(fd);if(!d)return-1;if(fd<3)return 0;if(!--d->refs)*d=(struct desc){0};fds[fd]=0;return 0;}

long read(int fd,void *buf,size_t n){
 struct desc *d=get(fd);if(!d)return-1;if(!n)return 0;if(!buf||n>LONG_MAX)return fail(EINVAL);
 if(d->kind==FD_IN){int c=getchar();if(c<0)return 0;*(unsigned char *)buf=c;return 1;}
 if(d->kind!=FD_ROM)return fail(EBADF);if(d->pos>=d->size)return 0;if(n>d->size-d->pos)n=d->size-d->pos;memcpy(buf,d->p+d->pos,n);d->pos+=n;return(long)n;
}

long write(int fd,const void *buf,size_t n){struct desc *d=get(fd);if(!d)return-1;if(d->kind!=FD_OUT&&d->kind!=FD_ERR)return fail(EBADF);if(!n)return 0;if(!buf||n>INT_MAX)return fail(EINVAL);return console_write(buf,(unsigned)n);}

long lseek(int fd,long off,int w){
 struct desc *d=get(fd);if(!d)return-1;if(d->kind!=FD_ROM)return fail(ESPIPE);long b=w==SEEK_SET?0:w==SEEK_CUR?(long)d->pos:w==SEEK_END?(long)d->size:-1;
 if(b<0||(off>0&&off>LONG_MAX-b)||off<-b)return fail(EINVAL);d->pos=b+off;return(long)d->pos;
}

int dup(int fd){struct desc *d=get(fd);if(!d)return-1;int n=slot();if(n<0)return-1;fds[n]=d;d->refs++;return n;}

static void fill(struct stat *s,size_t z,mode_t m,unsigned long ino){memset(s,0,sizeof *s);s->st_mode=m;s->st_size=z;s->st_ino=ino;s->st_nlink=1;s->st_blksize=512;s->st_blocks=(z+511)/512;}

int stat(const char *name,struct stat *s){
 char p[PATH_MAX];size_t n;const unsigned char *d;if(!s)return fail(EFAULT);if(path(name,p))return-1;unsigned long ino=1;for(const unsigned char *q=(void *)p;*q;q++)ino=ino*33+*q;
 if(isdir0(p)){fill(s,0,S_IFDIR|0555,ino);return 0;}if(!(d=find0(p,&n)))return fail(ENOENT);if(n>LONG_MAX)return fail(EOVERFLOW);fill(s,n,S_IFREG|0444,(unsigned long)(uintptr_t)d);return 0;
}
int lstat(const char *p,struct stat *s){return stat(p,s);}
int fstat(int fd,struct stat *s){struct desc *d=get(fd);if(!d)return-1;if(!s)return fail(EFAULT);if(d->kind==FD_ROM)fill(s,d->size,S_IFREG|0444,(unsigned long)(uintptr_t)d->p);else fill(s,0,S_IFCHR|0666,d->kind);return 0;}
int isatty(int fd){struct desc *d=get(fd);if(!d)return 0;if(d->kind!=FD_ROM)return 1;errno=ENOTTY;return 0;}
int access(const char *p,int m){struct stat s;if(m&~(R_OK|W_OK|X_OK))return fail(EINVAL);if(stat(p,&s))return-1;if(m&W_OK)return fail(EROFS);if((m&X_OK)&&!S_ISDIR(s.st_mode))return fail(EACCES);return 0;}
int chdir(const char *name){char p[PATH_MAX];if(path(name,p))return-1;if(!isdir0(p))return fail(ENOTDIR);strcpy(cwd,p);return 0;}
char *getcwd(char *p,size_t n){size_t k=strlen(cwd);if(!p){errno=EINVAL;return 0;}if(n<k+2){errno=ERANGE;return 0;}p[0]='/';memcpy(p+1,cwd,k+1);return p;}
int unlink(const char *p){(void)p;return fail(EROFS);}
int link(const char *a,const char *b){(void)a;(void)b;return fail(EROFS);}
