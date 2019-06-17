// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_EXIT_H
#define _VE_EXIT_H

#include <openenclave/bits/defs.h>
#include "syscall.h"

OE_INLINE void ve_exit(int status)
{
    ve_syscall(OE_SYS_exit, status);
}

#endif /* _VE_EXIT_H */
