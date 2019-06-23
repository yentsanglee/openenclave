// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "call.h"
#include <stdio.h>
#include <unistd.h>
#include "io.h"

static int _handle_call(int fd, ve_call_buf_t* buf)
{
    switch (buf->func)
    {
        case VE_FUNC_PING:
        {
            ve_handle_call_ping(buf->arg1, &buf->retval);
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

#include "../common/call.c"
