// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/device.h>
#include <openenclave/internal/netdb.h>
#include <openenclave/internal/host_resolver.h>
#include <openenclave/internal/resolver.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/hostbatch.h>
#include "../common/hostresolvargs.h"
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>

// The host resolver is not actually a device in the file descriptor sense.

#define DEVICE_ID_HOST_RESOLVER 42
/*
**==============================================================================
**
** host batch:
**
**==============================================================================
*/

oe_device_t* oe_get_hostresolv(void);
static oe_host_batch_t* _host_batch;
static oe_spinlock_t _lock;
void* memcpy(void* dst, const void* src, size_t len);

static void _atexit_handler()
{
    oe_spin_lock(&_lock);
    oe_host_batch_delete(_host_batch);
    _host_batch = NULL;
    oe_spin_unlock(&_lock);
}

static oe_host_batch_t* _get_host_batch(void)
{
    const size_t BATCH_SIZE = sizeof(oe_hostresolv_args_t) + OE_BUFSIZ;

    if (_host_batch == NULL)
    {
        oe_spin_lock(&_lock);

        if (_host_batch == NULL)
        {
            _host_batch = oe_host_batch_new(BATCH_SIZE);
            oe_atexit(_atexit_handler);
        }

        oe_spin_unlock(&_lock);
    }

    return _host_batch;
}

/*
**==============================================================================
**
** hostresolv operations:
**
**==============================================================================
*/

#define RESOLV_MAGIC 0x536f636b

typedef oe_hostresolv_args_t args_t;

typedef struct _resolv
{
    struct _oe_resolver base;
    uint32_t magic;
} resolv_t;

static resolv_t* _cast_resolv(const oe_resolver_t* device)
{
    resolv_t* resolv = (resolv_t*)device;

    if (resolv == NULL || resolv->magic != RESOLV_MAGIC)
        return NULL;

    return resolv;
}

static resolv_t _hostresolv;

static ssize_t _hostresolv_getnameinfo(
    oe_resolver_t* dev,
    const struct oe_sockaddr* sa,
    socklen_t salen,
    char* host,
    socklen_t hostlen,
    char* serv,
    socklen_t servlen,
    int flags)
{
    (void)dev;
    (void)sa;
    (void)salen;
    (void)host;
    (void)hostlen;
    (void)serv;
    (void)servlen;
    (void)flags;
    return OE_EAI_FAIL;
}

//
// We try return the sockaddr if it fits, but if it doesn't we return
// OE_EAI_OVERFLOW and the required size. IF the buffer is overflowed the caller
// needs to try _hostresolv_getaddrinfo with a suitably reallocated buffer
//
static ssize_t _hostresolv_getaddrinfo_r(
    oe_resolver_t* resolv,
    const char* node,
    const char* service,
    const struct oe_addrinfo* hints,
    struct oe_addrinfo* res,
    ssize_t* required_size)
{
    ssize_t ret = -1;
    args_t* args = NULL;
    oe_host_batch_t* batch = _get_host_batch();
    size_t buffer_required = 0;

    oe_errno = 0;
    (void)resolv;

    if (!batch)
    {
        oe_errno = EINVAL;
        goto done;
    }

    if (!node && !service)
    {
        oe_errno = EINVAL;
        goto done;
    }

    size_t nodelen = oe_strlen(node);
    size_t servicelen = oe_strlen(node);

    if (!(nodelen > 0) && !(servicelen > 0))
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Input */
    {
        // With a buffer big enough for a single addrinfo
        if (!(args = oe_host_batch_calloc(
                  batch,
                  sizeof(args_t) + sizeof(struct oe_addrinfo) +
                      (size_t)nodelen + (size_t)servicelen + 2)))
        {
            oe_errno = ENOMEM;
            goto done;
        }

        args->op = OE_HOSTRESOLV_OP_GETADDRINFO;
        args->u.getaddrinfosize.ret = -1;

        // We always pass at least a zero length node and service.
        if (nodelen > 0)
        {
            memcpy(args->buf, node, (size_t)nodelen);
            args->buf[nodelen] = '\0';
            args->u.getaddrinfosize.nodelen = (int32_t)nodelen;
        }
        else
        {
            args->buf[0] = '\0';
            args->u.getaddrinfosize.nodelen = 0;
        }

        if (servicelen > 0)
        {
            memcpy(args->buf + nodelen + 1, service, (size_t)servicelen);
            args->buf[nodelen + 1 + servicelen] = '\0';
            args->u.getaddrinfosize.servicelen = (int32_t)servicelen;
        }
        else
        {
            args->buf[nodelen + 1 + servicelen] = '\0';
            args->u.getaddrinfosize.servicelen = 0;
        }

        if (hints)
        {
            args->u.getaddrinfosize.hint_flags = hints->ai_flags;
            args->u.getaddrinfosize.hint_family = hints->ai_family;
            args->u.getaddrinfosize.hint_socktype = hints->ai_socktype;
            args->u.getaddrinfosize.hint_protocol = hints->ai_protocol;
        }
        else
        {
            args->u.getaddrinfosize.hint_flags =
                (OE_AI_V4MAPPED | OE_AI_ADDRCONFIG);
            args->u.getaddrinfosize.hint_family = OE_AF_UNSPEC;
            args->u.getaddrinfosize.hint_socktype = 0;
            args->u.getaddrinfosize.hint_protocol = 0;
        }
        args->u.getaddrinfosize.buffer_needed =
            (int32_t)buffer_required; // pass down the buffer that is available.
                                      // It is likely enough
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTRESOLVER, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (args->u.getaddrinfosize.ret < 0)
        {
            // If the error is OE_EAI_OVERFLOW. If not, we need to walk the
            // structure to see how much space it needs

            oe_errno = args->err;
            ret = args->u.getaddrinfosize.ret;
            goto done;
        }
        ret = args->u.getaddrinfosize.ret;
    }

    /* Output */
    {
        // did we get a valid result? If this was supposed to be a chain, then
        // no

        size_t retsize = 0;
        struct oe_addrinfo* arginfo = (struct oe_addrinfo*)args->buf;
        do
        {
            retsize += sizeof(struct oe_addrinfo);
            if (arginfo->ai_canonname)
            {
                retsize += oe_strlen(arginfo->ai_canonname);
            }
            if (arginfo->ai_addr)
            {
                retsize += sizeof(struct oe_sockaddr);
            }

            arginfo = arginfo->ai_next;

        } while (arginfo != NULL);

        if (retsize > (size_t)*required_size)
        {
            *required_size = (ssize_t)retsize;
            ret = OE_EAI_OVERFLOW;
            goto done;
        }

        struct oe_addrinfo* retinfo = res;
        arginfo = (struct oe_addrinfo*)args->buf;
        uint8_t* bufptr = args->buf; // byte pointer
        do
        {
            memcpy(res, arginfo, sizeof(struct oe_addrinfo));
            bufptr += sizeof(struct oe_addrinfo);

            if (arginfo->ai_addr)
            {
                retinfo->ai_addr = (struct oe_sockaddr*)(arginfo + 1);
                memcpy(
                    retinfo->ai_addr,
                    arginfo->ai_addr,
                    sizeof(struct oe_addrinfo));
                bufptr += sizeof(struct oe_sockaddr);
            }
            if (retinfo->ai_canonname)
            {
                bufptr += oe_strlen(retinfo->ai_canonname) + 1;
                memcpy(
                    retinfo->ai_canonname,
                    arginfo->ai_canonname,
                    oe_strlen(retinfo->ai_canonname) + 1);
            }

            retinfo->ai_next = (struct oe_addrinfo*)bufptr;
            retinfo = retinfo->ai_next;
            arginfo = arginfo->ai_next;

        } while (retinfo != NULL);
    }

done:

    if (args)
        oe_host_batch_free(batch);

    return ret;
}

static int _hostresolv_shutdown(oe_resolver_t* resolv_)
{
    int ret = -1;
    resolv_t* resolv = _cast_resolv(resolv_);
    oe_host_batch_t* batch = _get_host_batch();
    args_t* args = NULL;

    oe_errno = 0;

    /* Check parameters. */
    if (!resolv_ || !batch)
    {
        oe_errno = EINVAL;
        goto done;
    }

    /* Input */
    {
        if (!(args = oe_host_batch_calloc(batch, sizeof(args_t))))
        {
            oe_errno = ENOMEM;
            goto done;
        }

        args->op = OE_HOSTRESOLV_OP_SHUTDOWN;
        args->u.shutdown_device.ret = -1;
    }

    /* Call */
    {
        if (oe_ocall(OE_OCALL_HOSTRESOLVER, (uint64_t)args, NULL) != OE_OK)
        {
            oe_errno = EINVAL;
            goto done;
        }

        if (args->u.shutdown_device.ret != 0)
        {
            oe_errno = args->err;
            goto done;
        }
    }

    /* Release the resolv_ object. */
    oe_free(resolv);

    ret = 0;

done:
    return ret;
}

static oe_resolver_ops_t _ops = {.getaddrinfo_r = _hostresolv_getaddrinfo_r,
                                 .getnameinfo = _hostresolv_getnameinfo,
                                 .shutdown = _hostresolv_shutdown};

static resolv_t _hostresolv = {.base.type = OE_RESOLVER_HOST,
                               .base.size = sizeof(resolv_t),
                               .base.ops = &_ops,
                               .magic = RESOLV_MAGIC};

oe_resolver_t* _get_hostresolv(void)
{
    return &_hostresolv.base;
}
