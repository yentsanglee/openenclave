// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#define ftell musl_ftell
#include "../3rdparty/musl/musl/src/stdio/ftell.c"
