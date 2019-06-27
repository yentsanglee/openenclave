// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "syscall.h"
#include "time.h"

unsigned ve_sleep(unsigned int seconds)
{
    struct ve_timespec tv = {.tv_sec = seconds, .tv_nsec = 0};

    if (ve_nanosleep(&tv, &tv))
        return (unsigned)tv.tv_sec;

    return 0;
}
