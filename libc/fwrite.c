#include <stdio.h>
#include "stdio_impl.h"

/* Rename fwrite() since libc has its own. */
#define fwrite musl_fwrite

#undef weak_alias
#define weak_alias(...)

/* Include fwrite.c to get __fwritex() used by printf(). */
#include "../3rdparty/musl/musl/src/stdio/fwrite.c"
