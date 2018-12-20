// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include "stdio_impl.h"

int musl_ferror(FILE *f)
{
    FLOCK(f);
    int ret = !!(f->flags & F_ERR);
    FUNLOCK(f);
    return ret;
}

weak_alias(musl_ferror, ferror_unlocked);
weak_alias(musl_ferror, _IO_ferror_unlocked);
