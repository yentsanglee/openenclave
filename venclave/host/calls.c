// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "calls.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/io.h"
#include "hostmalloc.h"
#include "trace.h"

int ve_call_handler(void* handler_arg, int fd, ve_call_buf_t* buf)
{
    switch (buf->func)
    {
        case VE_FUNC_ABORT:
        {
            printf("enclave aborted\n");
            fflush(stdout);
            abort();
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
        case VE_FUNC_CALL_HOST_FUNCTION:
        {
            return ve_handle_call_host_function(handler_arg, fd, buf);
        }
        case VE_FUNC_OCALL:
        {
            return ve_handle_ocall(handler_arg, fd, buf);
        }
        default:
        {
            return -1;
        }
    }
}
