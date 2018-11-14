// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/enclavelibc.h>

int enclibc_rand(void)
{
#if defined(__linux__)

    uint64_t r;

    __asm__ volatile("rdrand %0\n\t" : "=r"(r));

    /* Return a positive integer */
    return (int)(r & 0x000000007fffffff);

#elif _MSC_VER

    union
    {
        int x;
        unsigned char bytes[4];
    }
    u;
    uint64_t r;

    while (_rdrand64_step(&r) == 0)
        ;

    u.bytes[0] = (r & 0xff);

    while (_rdrand64_step(&r) == 0)
        ;

    u.bytes[1] = (r & 0xff);

    while (_rdrand64_step(&r) == 0)
        ;

    u.bytes[2] = (r & 0xff);

    while (_rdrand64_step(&r) == 0)
        ;

    u.bytes[3] = (r & 0xff);

    return u.x & 0x7fffffff;

#endif
}
