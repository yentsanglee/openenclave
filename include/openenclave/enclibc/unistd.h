// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_UNISTD_H
#define _ENCLIBC_UNISTD_H

#include "bits/common.h"

ENCLIBC_EXTERNC_BEGIN

#define ENCLIBC_STDIN_FILENO 0
#define ENCLIBC_STDOUT_FILENO 1
#define ENCLIBC_STDERR_FILENO 2

void* enclibc_sbrk(intptr_t increment);

#if defined(ENCLIBC_NEED_STDC_NAMES)

#define STDIN_FILENO ENCLIBC_STDIN_FILENO
#define STDOUT_FILENO ENCLIBC_STDOUT_FILENO
#define STDERR_FILENO ENCLIBC_STDERR_FILENO

ENCLIBC_INLINE
void* sbrk(intptr_t increment)
{
    return enclibc_sbrk(increment);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_UNISTD_H */
