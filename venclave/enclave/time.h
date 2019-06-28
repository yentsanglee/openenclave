// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_TIME_H
#define _VE_ENCLAVE_TIME_H

#include "common.h"

struct ve_timespec
{
    time_t tv_sec;
    long tv_nsec;
};

int ve_nanosleep(const struct ve_timespec* req, struct ve_timespec* rem);

unsigned ve_sleep(unsigned int seconds);

#endif /* _VE_ENCLAVE_TIME_H */
