// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "stdio_impl.h"

void musl_clearerr(FILE* f)
{
    FLOCK(f);
    f->flags &= ~(F_EOF | F_ERR);
    FUNLOCK(f);
}

weak_alias(musl_clearerr, clearerr_unlocked);
