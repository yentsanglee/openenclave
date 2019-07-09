// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include "calls.h"
#include "panic.h"
#include "socket.h"
#include "string.h"

void* oe_host_malloc(size_t size)
{
    uint64_t retval = 0;
    const int sock = ve_get_sock();

    if (ve_call1(sock, VE_FUNC_MALLOC, &retval, size) != 0)
        return NULL;

    return (void*)retval;
}

void* oe_host_calloc(size_t nmemb, size_t size)
{
    uint64_t retval = 0;
    const int sock = ve_get_sock();

    if (ve_call2(sock, VE_FUNC_CALLOC, &retval, nmemb, size) != 0)
        return NULL;

    return (void*)retval;
}

void* oe_host_realloc(void* ptr, size_t size)
{
    uint64_t retval = 0;
    const int sock = ve_get_sock();

    if (ve_call2(sock, VE_FUNC_REALLOC, &retval, (uint64_t)ptr, size) != 0)
        return NULL;

    return (void*)retval;
}

void oe_host_free(void* ptr)
{
    const int sock = ve_get_sock();

    if (ve_call1(sock, VE_FUNC_FREE, NULL, (uint64_t)ptr) != 0)
        ve_panic("oe_host_free() failed");
}

char* oe_host_strndup(const char* str, size_t n)
{
    char* p;
    size_t len;

    if (!str)
        return NULL;

    len = oe_strlen(str);

    if (n < len)
        len = n;

    /* Would be an integer overflow in the next statement. */
    if (len == OE_SIZE_MAX)
        return NULL;

    if (!(p = oe_host_malloc(len + 1)))
        return NULL;

    if (memcpy(p, str, len) != OE_OK)
        return NULL;

    p[len] = '\0';

    return p;
}
