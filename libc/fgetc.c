// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#define fgetc musl_fgetc
#include "../3rdparty/musl/musl/src/stdio/fgetc.c"
