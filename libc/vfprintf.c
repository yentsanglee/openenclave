#include <stdio.h>
#include "stdio_impl.h"

#define vfprintf musl_vfprintf
#include "../3rdparty/musl/musl/src/stdio/vfprintf.c"
