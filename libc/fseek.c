// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#define fseek musl_fseek
#include "../3rdparty/musl/musl/src/stdio/fseek.c"
