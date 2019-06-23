// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../common/call.h"
#include "call.h"
#include "io.h"

static int _handle_call(
    int fd,
    uint64_t func,
    uint64_t arg_in,
    uint64_t* arg_out)
{
    switch (func)
    {
        case VE_FUNC_PING:
        {
            ve_handle_call_ping(fd, arg_in, arg_out);
            return 0;
        }
        case VE_FUNC_ADD_THREAD:
        {
            if (!arg_in)
                return -1;

            ve_handle_call_add_thread(arg_in);
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

int ve_handle_calls(int fd)
{
    int ret = -1;

    if (fd < 0)
        goto done;

    for (;;)
    {
        ve_call_msg_t msg_in;
        ve_call_msg_t msg_out;
        uint64_t arg = 0;

        if (ve_readn(fd, &msg_in, sizeof(msg_in)) != 0)
            goto done;

#if defined(TRACE_CALLS)
        ve_put_s("ENCLAVE:", ve_func_name(msg_in.func));
#endif

        if (_handle_call(fd, msg_in.func, msg_in.arg, &arg) == 0)
        {
            msg_out.func = VE_FUNC_RET;
            msg_out.arg = arg;
        }
        else
        {
            msg_out.func = VE_FUNC_ERR;
            msg_out.arg = 0;
        }

        if (ve_writen(fd, &msg_out, sizeof(msg_out)) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

int ve_call(int fd, uint64_t func, uint64_t arg_in, uint64_t* arg_out)
{
    int ret = -1;

    if (arg_out)
        *arg_out = 0;

    if (fd < 0)
        goto done;

    /* Send request. */
    {
        ve_call_msg_t msg = {func, arg_in};

        if (ve_writen(fd, &msg, sizeof(msg)) != 0)
            goto done;
    }

    /* Receive response. */
    for (;;)
    {
        ve_call_msg_t msg;

        if (ve_readn(fd, &msg, sizeof(msg)) != 0)
            goto done;

        switch (msg.func)
        {
            case VE_FUNC_RET:
            {
                if (arg_out)
                    *arg_out = msg.arg;

                ret = 0;
                goto done;
            }
            case VE_FUNC_ERR:
            {
                goto done;
            }
            default:
            {
                uint64_t arg = 0;

                if (_handle_call(fd, msg.func, msg.arg, &arg) == 0)
                {
                    msg.func = VE_FUNC_RET;
                    msg.arg = arg;
                }
                else
                {
                    msg.func = VE_FUNC_ERR;
                    msg.arg = 0;
                }

                if (ve_writen(fd, &msg, sizeof(msg)) != 0)
                    goto done;

                /* Go back to waiting for return from original call. */
                continue;
            }
        }
    }

done:
    return ret;
}

#include "../common/call.c"
