// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_TIME_H
#define _ENCLIBC_TIME_H

#include "bits/common.h"
#include "sys/time.h"

ENCLIBC_EXTERNC_BEGIN

struct enclibc_tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct enclibc_timespec
{
    time_t tv_sec;
    long tv_nsec;
};

time_t enclibc_time(time_t* tloc);

struct enclibc_tm* enclibc_gmtime(const time_t* timep);

struct enclibc_tm* enclibc_gmtime_r(
    const time_t* timep,
    struct enclibc_tm* result);

#if defined(ENCLIBC_NEED_STDC_NAMES)

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};

ENCLIBC_INLINE
time_t time(time_t* tloc)
{
    return enclibc_time(tloc);
}

ENCLIBC_INLINE
struct tm* gmtime(const time_t* timep)
{
    return (struct tm*)enclibc_gmtime(timep);
}

ENCLIBC_INLINE
struct tm* gmtime_r(const time_t* timep, struct tm* result)
{
    return (struct tm*)enclibc_gmtime_r(timep, (struct enclibc_tm*)result);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_TIME_H */
