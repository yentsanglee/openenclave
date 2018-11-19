#include <stdio.h>
#include "stdio_impl.h"

#define fgets musl_fgets
#undef weak_alias
#define weak_alias(...)
#include "../3rdparty/musl/musl/src/stdio/fgets.c"
#include "weak_alias.h"

weak_alias(musl_fgets, fgets_unlocked);
