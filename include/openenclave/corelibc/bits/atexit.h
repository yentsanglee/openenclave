// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_BITS_ATEXIT_H
#define _OE_BITS_ATEXIT_H

#include <openenclave/corelibc/bits/common.h>

OE_INLINE
int atexit(void (*function)(void))
{
    return oe_atexit(function);
}

#endif /* _OE_BITS_ATEXIT_H */
