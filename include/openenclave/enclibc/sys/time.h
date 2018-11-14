// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_SYS_TIME_H
#define _ENCLIBC_SYS_TIME_H

#include "../bits/common.h"

ENCLIBC_EXTERNC_BEGIN

struct enclibc_timeval
{
    time_t tv_sec;
    suseconds_t tv_usec;
};

struct enclibc_timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

#if defined(ENCLIBC_NEED_STDC_NAMES)

struct timeval
{
    time_t tv_sec;
    suseconds_t tv_usec;
};

struct timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_SYS_TIME_H */
