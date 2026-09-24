#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

typedef long long intmax_t;
typedef unsigned long long uintmax_t;

#define PRId32 "d"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIdMAX "lld"
#define PRIuMAX "llu"
#define PRIxMAX "llx"

#endif
