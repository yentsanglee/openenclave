// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "call.h"
#include <stdio.h>
#include <unistd.h>
#include "io.h"

static int _handle_call(uint64_t func, uint64_t arg_in, uint64_t* arg_out)
{
    switch (func)
    {
        case VE_FUNC_PING:
        {
            ve_handle_call_ping(arg_in, arg_out);
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

int ve_call_send(int fd, uint64_t func, uint64_t arg_in)
{
    int ret = -1;

    if (fd < 0)
        goto done;

    /* Send request. */
    {
        ve_call_msg_t msg = {func, arg_in};

        if (ve_writen(fd, &msg, sizeof(msg)) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

int ve_call_recv(int fd, uint64_t* arg_out)
{
    int ret = -1;

    if (arg_out)
        *arg_out = 0;

    if (fd < 0)
        goto done;

    /* Receive response. */
    for (;;)
    {
        ve_call_msg_t msg;

        if (ve_readn(fd, &msg, sizeof(msg)) != 0)
            goto done;

#if defined(TRACE_CALLS)
        printf("HOST:%s\n", ve_func_name(msg.func));
#endif

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

                if (_handle_call(msg.func, msg.arg, &arg) == 0)
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

int ve_call(int fd, uint64_t func, uint64_t arg_in, uint64_t* arg_out)
{
    int ret = -1;

    if (ve_call_send(fd, func, arg_in) != 0)
        goto done;

    if (ve_call_recv(fd, arg_out) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

#include "../common/call.c"
