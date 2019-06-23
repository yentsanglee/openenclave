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
            ve_handle_call_ping(fd, buf);
            return 0;
        }
        case VE_FUNC_ADD_THREAD:
        {
            ve_handle_call_add_thread(fd, buf);
            return 0;
        }
        case VE_FUNC_TERMINATE:
        {
            ve_handle_call_terminate(fd, buf);
            return 0;
        }
        case VE_FUNC_TERMINATE_THREAD:
        {
            ve_handle_call_terminate_thread(fd, buf);
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

        if (ve_readn(fd, &in, sizeof(in)) != 0)
            goto done;

#if defined(TRACE_CALLS)
        ve_print("[ENCLAVE:%s]", ve_func_name(in.func));
#endif

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
    }

    ret = 0;

done:
    VE_TRACE;
    return ret;
}

#include "../common/call.c"
