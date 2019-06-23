// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "call.h"
#include <stdio.h>
#include <unistd.h>
#include "hostmalloc.h"
#include "io.h"
#include "trace.h"

static int _handle_call(int fd, ve_call_buf_t* buf)
{
    switch (buf->func)
    {
        case VE_FUNC_PING:
        {
            ve_handle_call_ping(buf);
            return 0;
        }
        case VE_FUNC_MALLOC:
        {
            buf->retval = (uint64_t)ve_host_malloc((size_t)buf->arg1);
            return 0;
        }
        case VE_FUNC_CALLOC:
        {
            buf->retval =
                (uint64_t)ve_host_calloc((size_t)buf->arg1, (size_t)buf->arg2);
            return 0;
        }
        case VE_FUNC_REALLOC:
        {
            buf->retval =
                (uint64_t)ve_host_realloc((void*)buf->arg1, (size_t)buf->arg2);
            return 0;
        }
        case VE_FUNC_FREE:
        {
            ve_host_free((void*)buf->arg1);
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

#include "../common/call.c"
