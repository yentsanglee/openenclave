// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_INTERNAL_SWITCHLESS_H
#define _OE_INTERNAL_SWITCHLESS_H

OE_EXTERNC_BEGIN

#include <openenclave/bits/defs.h>
#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>

typedef struct _switchless_call_context
{
    void* volatile args;
    volatile bool shutdown_initiated;
    volatile bool shutdown_complete;
} oe_switchless_context_t;

OE_EXTERNC_END

#endif // _OE_INTERNAL_SWITCHLESS_H
