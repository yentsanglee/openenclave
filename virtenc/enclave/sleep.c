// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/time.h>
#include <openenclave/internal/syscall/sys/syscall.h>
#include "syscall.h"
#include "time.h"

unsigned ve_sleep(unsigned int seconds)
{
    struct oe_timespec tv = {.tv_sec = seconds, .tv_nsec = 0};

    if (ve_nanosleep(&tv, &tv))
        return (unsigned)tv.tv_sec;

    return 0;
}
