// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../common/call.h"
#include "call.h"
#include "globals.h"
#include "io.h"
#include "string.h"
#include "trace.h"

static int _handle_call(int fd, ve_call_buf_t* buf)
{
    switch (buf->func)
    {
        case VE_FUNC_PING:
        {
            ve_handle_call_ping(fd, buf->arg1, &buf->retval);
            return 0;
        }
        case VE_FUNC_ADD_THREAD:
        {
            if (!buf->arg1)
                return -1;

            ve_handle_call_add_thread(buf->arg1);
            return 0;
        }
        case VE_FUNC_TERMINATE:
        {
            /* does not return. */
            ve_handle_call_terminate();
            return 0;
        }
        case VE_FUNC_TERMINATE_THREAD:
        {
            /* does not return. */
            ve_handle_call_terminate_thread();
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

void* ve_call_malloc(size_t size)
{
    uint64_t retval = 0;

    if (ve_call1(globals.sock, VE_FUNC_MALLOC, &retval, size) != 0)
        return NULL;

    return (void*)retval;
}

int ve_handle_calls(int fd)
{
    int ret = -1;

    if (fd < 0)
        goto done;

    for (;;)
    {
        ve_call_buf_t buf_in;
        ve_call_buf_t buf_out;

        if (ve_readn(fd, &buf_in, sizeof(buf_in)) != 0)
            goto done;

#if defined(TRACE_CALLS)
        ve_print("[ENCLAVE:%s]", ve_func_name(buf_in.func));
#endif

        ve_call_buf_clear(&buf_out);

        if (_handle_call(fd, &buf_in) == 0)
        {
            buf_out.func = VE_FUNC_RET;
            buf_out.retval = buf_in.retval;
        }
        else
        {
            buf_out.func = VE_FUNC_ERR;
            buf_out.retval = 0;
        }

        if (ve_writen(fd, &buf_out, sizeof(buf_out)) != 0)
            goto done;
    }

    ret = 0;

done:
    VE_TRACE;
    return ret;
}

#include "../common/call.c"
