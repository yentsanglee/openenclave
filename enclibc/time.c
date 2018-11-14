// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <time.h>
#include <openenclave/internal/time.h>

time_t enclibc_time(time_t* tloc)
{
    uint64_t msec = oe_get_time();
    /* ATTN: size conversion? */
    time_t time = (time_t)(msec / 1000);

    if (*tloc)
        *tloc = time;

    return time;
}
