// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/time.h>
#include <openenclave/internal/syscall/sys/syscall.h>
#include "syscall.h"
#include "time.h"

int ve_nanosleep(const struct oe_timespec* req, struct oe_timespec* rem)
{
    return ve_syscall(OE_SYS_nanosleep, req, rem);
}
