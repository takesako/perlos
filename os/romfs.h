#ifndef ROMFS_H
#define ROMFS_H

/*
 * Simple read-only ROMFS v2 format.
 *
 * - No directory tree. Names are relative paths such as "lib/strict.pm".
 * - Names are variable length and matched by length + byte comparison.
 * - Integers use native endianness.
 * - No compression.
 * - Each name and file is followed by a NUL byte, not included in its length.
 * - Entries, names and file data are aligned to 4 bytes.
 * - Read-only: writing, deleting, and appending are not supported.
 *
 * Layout:
 *   [romfs_header]
 *   [romfs_entry][name + NUL][pad][file data + NUL][pad]
 *   [romfs_entry][name + NUL][pad][file data + NUL][pad]
 *   ...
 */

#include <stdint.h>
#include <stddef.h>

#define ROMFS_MAGIC "RFS2"

struct romfs_header {
    char magic[4];
    uint32_t total_size;
};

struct romfs_entry {
    uint32_t name_len;
    uint32_t size;
    char name[];
};

int romfs_init(const void *);
int romfs_isdir(const char *);
void romfs_each(void (*fn)(const char *,void *),void *arg);

const unsigned char *_romfs_find(const char *, size_t *);
const unsigned char *romfs_data_for_compile(const char *, size_t *);

#endif
