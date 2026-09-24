#include <stdio.h>
#include "romfs.h"
#include "perlos.h"

extern const unsigned char romfs_image[];

int main(void)
{
    if (romfs_init(romfs_image)) {
        puts("ERROR: romfs_init() failed!");
        return 1;
    }
    return perlos_repl();
}
