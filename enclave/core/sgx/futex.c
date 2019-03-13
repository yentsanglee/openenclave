// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/sgx/sgxproperties.h>
#include <openenclave/corelibc/errno.h>
#include <openenclave/corelibc/linux/futex.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include "td.h"

extern volatile const oe_sgx_enclave_properties_t oe_enclave_properties_sgx;

typedef struct _proxy
{
    int* enclave_uaddr;
    int* host_uaddr;
    uint64_t refs;
} proxy_t;

static proxy_t* _proxies;
static size_t _num_proxies;
static size_t _max_proxies;
static void** _host_uaddrs;
oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;
oe_once_t _once = OE_ONCE_INITIALIZER;

static void _initialize(void)
{
    oe_spin_lock(&_lock);

    _max_proxies = oe_enclave_properties_sgx.header.size_settings.num_tcs;

    if (!(_proxies = oe_calloc(_max_proxies, sizeof(proxy_t))))
        oe_abort();

    if (!(_host_uaddrs = oe_host_calloc(_max_proxies, sizeof(void*))))
        oe_abort();

    for (size_t i = 0; i < _max_proxies; i++)
        _proxies[i].host_uaddr = (int*)&_host_uaddrs[i];

    oe_spin_unlock(&_lock);
}

int* _get_proxy(int* enclave_uaddr)
{
    int* host_uaddr = NULL;

    oe_once(&_once, _initialize);
    oe_spin_lock(&_lock);

    /* Search for an existing proxy. */
    for (size_t i = 0; i < _num_proxies; i++)
    {
        proxy_t* proxy = &_proxies[i];

        if (proxy->enclave_uaddr == enclave_uaddr)
        {
            host_uaddr = proxy->host_uaddr;
            *host_uaddr = *enclave_uaddr;
            proxy->refs++;
            goto done;
        }
    }

    /* An impossible condition. */
    if (_num_proxies == _max_proxies)
        oe_abort();

    /* Assign a new proxy. */
    {
        proxy_t* proxy = &_proxies[_num_proxies++];
        proxy->enclave_uaddr = enclave_uaddr;
        proxy->refs = 1;
        host_uaddr = proxy->host_uaddr;
        *host_uaddr = *enclave_uaddr;
        goto done;
    }

done:

    oe_spin_unlock(&_lock);

    return host_uaddr;
}

int _put_proxy(int* host_uaddr)
{
    int ret = -1;

    oe_spin_lock(&_lock);

    /* Find and release the proxy. */
    for (size_t i = 0; i < _num_proxies; i++)
    {
        proxy_t* proxy = &_proxies[i];

        if (proxy->host_uaddr == host_uaddr)
        {
            if (proxy->refs == 0)
                oe_abort();

            if (--proxy->refs == 0)
            {
                _proxies[i] = _proxies[_num_proxies - 1];
                *_proxies[i].host_uaddr = 0;
                _num_proxies--;
            }

            goto done;
        }
    }

    ret = 0;

done:

    oe_spin_unlock(&_lock);

    return ret;
}

int oe_futex(
    int* uaddr,
    int futex_op,
    int val,
    const struct oe_timespec* timeout,
    int* uaddr2,
    int val3)
{
    int ret = -1;
    oe_futex_args_t* args = NULL;
    td_t* td;
    int* host_uaddr = NULL;

    /* Assign a host proxy address for this enclave address. */
    if (!(host_uaddr = _get_proxy(uaddr)))
        goto done;

    oe_printf("futex.enclave(): %d %d %p %p\n", *uaddr, val, uaddr, host_uaddr);

    if (!(td = oe_get_td()))
        goto done;

    if (!(args = oe_host_calloc(1, sizeof(*args))))
        goto done;

    /* Input */
    {
        args->ret = -1;
        args->err = 0;

        if (!(args->tcs = (uint64_t)td_to_tcs(td)))
            goto done;

        args->uaddr = host_uaddr;
        args->futex_op = futex_op;
        args->val = val;

        if (timeout)
        {
            memcpy(&args->timeout_buf, timeout, sizeof(args->timeout_buf));
            args->timeout = &args->timeout_buf;
        }
        else
        {
            memset(&args->timeout_buf, 0, sizeof(args->timeout_buf));
            args->timeout = NULL;
        }

        args->uaddr2 = uaddr2;
        args->val3 = val3;
    }

    oe_printf("futex.case4\n");
    /* Call */
    if (oe_ocall(OE_OCALL_FUTEX, (uint64_t)args, NULL) != OE_OK)
    {
        oe_printf("futex.case5\n");
        goto done;
    }
    oe_printf("futex.case6\n");

    /* Output */
    oe_errno = args->err;
    ret = (int)args->ret;

done:

    oe_printf("futex.case5\n");
    if (host_uaddr)
        _put_proxy(host_uaddr);

    if (args)
        oe_host_free(args);

    return ret;
}
