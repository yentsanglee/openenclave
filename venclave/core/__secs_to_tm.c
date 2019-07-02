// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

#define INT_MIN OE_INT_MIN
#define INT_MAX OE_INT_MAX

#pragma GCC diagnostic ignored "-Wshorten-64-to-32"

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
    long __tm_gmtoff;
    const char* __tm_zone;
};

#include "../../enclave/core/__secs_to_tm.c"
