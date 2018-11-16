#include "stdio_impl.h"

void musl_clearerr(FILE *f)
{
    FLOCK(f);
    f->flags &= ~(F_EOF|F_ERR);
    FUNLOCK(f);
}

weak_alias(musl_clearerr, clearerr_unlocked);
