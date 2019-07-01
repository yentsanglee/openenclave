// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "time.h"
#include "print.h"
#include "syscall.h"

unsigned ve_sleep(unsigned int seconds)
{
    struct ve_timespec tv = {.tv_sec = seconds, .tv_nsec = 0};

    if (ve_nanosleep(&tv, &tv))
        return (unsigned)tv.tv_sec;

    return 0;
}

int ve_nanosleep(const struct ve_timespec* req, struct ve_timespec* rem)
{
    return (int)ve_syscall2(VE_SYS_nanosleep, (long)req, (long)rem);
}

int ve_clock_gettime(ve_clockid_t clk_id, struct ve_timespec* tp)
{
    int ret = -1;

    if (ve_syscall2(VE_SYS_clock_gettime, clk_id, (long)tp) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}
