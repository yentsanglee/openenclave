// clang-format off
#include <stdio.h>
#include "stdio_impl.h"

#define fopen musl_fopen
#include "../3rdparty/musl/musl/src/stdio/fopen.c"
