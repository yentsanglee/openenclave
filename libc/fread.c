#include <stdio.h>
#include "stdio_impl.h"

#define fread musl_fread
#undef weak_alias
#define weak_alias(...)
#include "../3rdparty/musl/musl/src/stdio/fread.c"
#include "weak_alias.h"

weak_alias(musl_fread, fread_unlocked);
