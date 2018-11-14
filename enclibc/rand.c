// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <setjmp.h>
#include <openenclave/internal/enclavelibc.h>

/* ATTN: WINPORT */
#if defined(__linux__)
int enclibc_rand(void)
{
    uint64_t r;

    __asm__ volatile("rdrand %0\n\t" : "=r"(r));

    /* Return a positive integer */
    return (int)(r & 0x000000007fffffff);
}
#endif
