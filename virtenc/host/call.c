// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "call.h"
#include <stdio.h>
#include <unistd.h>
#include "io.h"

static int _handle_call(int fd, uint64_t func, uint64_t arg1, uint64_t* retval)
{
    switch (func)
    {
        case VE_FUNC_PING:
        {
            ve_handle_call_ping(arg1, retval);
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

#include "../common/call.c"
