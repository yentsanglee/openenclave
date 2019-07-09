// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "calls.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../common/io.h"
#include "hostmalloc.h"
#include "trace.h"

static int _handle_call(int fd, ve_call_buf_t* buf)
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
            return ve_handle_call_host_function(fd, buf);
        }
        case VE_FUNC_OCALL:
        {
            return ve_handle_ocall(fd, buf);
        }
        default:
        {
            return -1;
        }
    }
}

int ve_call_recv(int fd, uint64_t* retval)
{
    int ret = -1;

    if (retval)
        *retval = 0;

    if (fd < 0)
        goto done;

    /* Receive response. */
    for (;;)
    {
        ve_call_buf_t in;

        if (ve_readn(fd, &in, sizeof(in)) != 0)
            goto done;

        switch (in.func)
        {
            case VE_FUNC_RET:
            {
                if (retval)
                    *retval = in.retval;

                ret = 0;
                goto done;
            }
            case VE_FUNC_ERR:
            {
                goto done;
            }
            default:
            {
                ve_call_buf_t out;

                ve_call_buf_clear(&out);

                if (_handle_call(fd, &in) == 0)
                {
                    out.func = VE_FUNC_RET;
                    out.retval = in.retval;
                }
                else
                {
                    out.func = VE_FUNC_ERR;
                }

                if (ve_writen(fd, &out, sizeof(out)) != 0)
                    goto done;

                /* Go back to waiting for return from original call. */
                continue;
            }
        }
    }

done:
    return ret;
}
