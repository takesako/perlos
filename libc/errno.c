#include "errno.h"

static int e;
int *__errno(void){return &e;}
