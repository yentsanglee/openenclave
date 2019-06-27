// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../common/call.h"
#include "call.h"
#include "common.h"
#include "globals.h"
#include "io.h"
#include "malloc.h"
#include "string.h"
#include "trace.h"

static int _handle_call(int fd, ve_call_buf_t* buf, int* exit_status)
{
    if (exit_status)
        *exit_status = 0;

    switch (buf->func)
    {
        case VE_FUNC_PING:
        {
            return ve_handle_call_ping(fd, buf, exit_status);
        }
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
        case VE_FUNC_ECALL:
        {
            return ve_handle_call_ecall(fd, buf, exit_status);
        }
        case VE_FUNC_INIT_ENCLAVE:
        {
            return ve_handle_init_enclave(fd, buf, exit_status);
        }
        case VE_FUNC_XOR:
        {
            uint64_t x1 = buf->arg1;
            uint64_t x2 = buf->arg2;
            uint64_t x3 = buf->arg3;
            uint64_t x4 = buf->arg4;
            uint64_t x5 = buf->arg5;
            uint64_t x6 = buf->arg6;
            buf->retval = (x1 ^ x2 ^ x3 ^ x4 ^ x5 ^ x6);
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

void* ve_call_malloc(int fd, size_t size)
{
    uint64_t retval = 0;

    if (ve_call1(fd, VE_FUNC_MALLOC, &retval, size) != 0)
        return NULL;

    return (void*)retval;
}

void* ve_call_calloc(int fd, size_t nmemb, size_t size)
{
    uint64_t retval = 0;

    if (ve_call2(fd, VE_FUNC_CALLOC, &retval, nmemb, size) != 0)
        return NULL;

    return (void*)retval;
}

void* ve_call_realloc(int fd, void* ptr, size_t size)
{
    uint64_t retval = 0;

    if (ve_call2(fd, VE_FUNC_REALLOC, &retval, (uint64_t)ptr, size) != 0)
        return NULL;

    return (void*)retval;
}

void* ve_call_memalign(int fd, size_t alignment, size_t size)
{
    uint64_t retval = 0;

    if (ve_call2(fd, VE_FUNC_MEMALIGN, &retval, alignment, size) != 0)
        return NULL;

    return (void*)retval;
}

int ve_call_free(int fd, void* ptr)
{
    if (ve_call1(fd, VE_FUNC_FREE, NULL, (uint64_t)ptr) != 0)
        return -1;

    return 0;
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
        int exit_status = 0;

        if (ve_readn(fd, &in, sizeof(in)) != 0)
            goto done;

#if defined(TRACE_CALLS)
        ve_print("[ENCLAVE:%s]\n", ve_func_name(in.func));
#endif

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
                /* The handler exited. */
                ret = exit_status;
                goto done;
            }
        }

        if (ve_writen(fd, &out, sizeof(out)) != 0)
            goto done;
    }

done:
    return ret;
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
                goto done;
            }
        }
    }

done:
    return ret;
}

#include "../common/call.c"
