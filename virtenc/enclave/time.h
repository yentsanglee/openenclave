// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_TIME_H
#define _VE_TIME_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include <openenclave/corelibc/time.h>

int ve_nanosleep(const struct oe_timespec* req, struct oe_timespec* rem);

unsigned ve_sleep(unsigned int seconds);

#endif /* _VE_TIME_H */
