// clang-format off
#include "stdio_impl.h"

int musl_feof(FILE *f)
{
    FLOCK(f);
    int ret = !!(f->flags & F_EOF);
    FUNLOCK(f);
    return ret;
}

weak_alias(musl_feof, feof_unlocked);
weak_alias(musl_feof, _IO_feof_unlocked);
