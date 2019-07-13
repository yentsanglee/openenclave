// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "calls.h"
#include "common.h"
#include "io.h"
#include "malloc.h"
#include "panic.h"
#include "process.h"
#include "socket.h"
#include "string.h"
#include "thread.h"
#include "trace.h"

static int _handle_call(int fd, ve_call_buf_t* buf, int* exit_status)
{
    switch (buf->func)
    {
        case VE_FUNC_ADD_THREAD:
        {
            return ve_handle_call_add_thread(fd, buf, exit_status);
        }
        case VE_FUNC_TERMINATE:
        {
            return ve_handle_call_terminate(fd, buf, exit_status);
        }
        case VE_FUNC_TERMINATE_THREAD:
        {
            return ve_handle_call_terminate_thread(fd, buf, exit_status);
        }
        case VE_FUNC_GET_SETTINGS:
        {
            return ve_handle_get_settings(fd, buf, exit_status);
        }
        case VE_FUNC_INIT_ENCLAVE:
        {
            return ve_handle_init_enclave(fd, buf, exit_status);
        }
        case VE_FUNC_CALL_ENCLAVE_FUNCTION:
        {
            return ve_handle_call_enclave_function(fd, buf, exit_status);
        }
        default:
        {
            return -1;
        }
    }
}

int ve_handle_calls(int fd)
{
    int ret = -1;

    if (fd < 0)
        goto done;

    for (;;)
    {
        ve_call_buf_t in;
        ve_call_buf_t out;
        int retval;
        int exit_status = -1;

        if (ve_readn(fd, &in, sizeof(in)) != 0)
            goto done;

        ve_call_buf_clear(&out);

        switch ((retval = _handle_call(fd, &in, &exit_status)))
        {
            case 0:
            {
                out.func = VE_FUNC_RET;
                out.retval = in.retval;
                break;
            }
            case -1:
            {
                out.func = VE_FUNC_ERR;
                break;
            }
            default:
            {
                ve_panic("unexpected");
            }
        }

        if (ve_writen(fd, &out, sizeof(out)) != 0)
            goto done;

        if (exit_status >= 0)
        {
            ve_close(fd);
            ret = exit_status;
            break;
        }
    }

done:
    return ret;
}
