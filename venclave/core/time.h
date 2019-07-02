// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_TIME_H
#define _VE_ENCLAVE_TIME_H

#include "common.h"

#define VE_CLOCK_REALTIME 0

struct ve_timespec
{
    time_t tv_sec;
    long tv_nsec;
};

typedef int ve_clockid_t;

int ve_nanosleep(const struct ve_timespec* req, struct ve_timespec* rem);

unsigned ve_sleep(unsigned int seconds);

int ve_clock_gettime(ve_clockid_t clk_id, struct ve_timespec* tp);

#endif /* _VE_ENCLAVE_TIME_H */
