#include <stdio.h>
#include <string.h>
#include "romfs.h"

extern const unsigned char romfs_image[];
extern const unsigned long romfs_image_size;

static int n,fail;
#define ok(c,s) do{n++;printf("%s %d - %s\n",(c)?"ok":"not ok",n,s);if(!(c))fail++;}while(0)

int main(void)
{
    size_t z,m;
    const unsigned char *p,*q;

    printf("1..10\n");

    ok(romfs_init(romfs_image)==0,"romfs_init");
    ok(((const struct romfs_header*)romfs_image)->total_size==romfs_image_size,"image size");

    p=_romfs_find("hello.txt",&z);
    ok(p!=0,"find hello.txt");
    ok(z==13,"hello.txt size");
    ok(p&&!memcmp(p,"Hello ROMFS!\n",13),"hello.txt data");
    ok(p&&p[z]==0,"hello.txt NUL");

    p=_romfs_find("lib/Test.pm",&z);
    ok(p&&z==17&&!memcmp(p,"package Test;\n1;\n",17),"lib/Test.pm data");
    ok(p&&p[z]==0,"lib/Test.pm NUL");

    q=romfs_data_for_compile("lib/Test.pm",&m);
    ok(q==p&&m==z,"compile data");

    ok(!_romfs_find("missing",&z),"missing file");

    return fail!=0;
}
