// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_SYS_FUTEX_H
#define _OE_SYS_FUTEX_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/time.h>

OE_EXTERNC_BEGIN

int oe_futex(
    int* uaddr,
    int futex_op,
    int val,
    const struct oe_timespec* timeout,
    int* uaddr2,
    int val3);

#if defined(OE_NEED_STDC_NAMES)

#endif /* defined(OE_NEED_STDC_NAMES) */

OE_EXTERNC_END

#endif /* _OE_SYS_FUTEX_H */
