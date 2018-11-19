// clang-format off
#include <stdio.h>
#include "stdio_impl.h"
#undef weak_alias
#define weak_alias(...)
#define putc musl_putc
#include "../3rdparty/musl/musl/src/stdio/putc.c"
#include "weak_alias.h"

weak_alias(musl_putc, _IO_putc);
