// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_RESOLVER_H
#define _OE_RESOLVER_H

#include <openenclave/bits/types.h>
#include <openenclave/internal/errno.h>

OE_EXTERNC_BEGIN

typedef uint32_t oe_socklen_t;
typedef struct oe_addrinfo
{
    int32_t ai_flags;
    int32_t ai_family;
    int32_t ai_socktype;
    int32_t ai_protocol;
    size_t ai_addrlen;
    struct oe_sockaddr* ai_addr;
    char* ai_canonname;
    struct oe_addrinfo* ai_next;
} oe_addrinfo;

typedef struct _oe_resolver oe_resolver_t;

typedef struct _oe_resolver_ops
{
    int (*getaddrinfo)(
        oe_resolver_t* dev,
        const char* node,
        const char* service,
        const struct oe_addrinfo* hints,
        struct oe_addrinfo** res);

    void (*freeaddrinfo)(oe_resolver_t* dev, struct oe_addrinfo* res);

    int (*getnameinfo)(
        oe_resolver_t* dev,
        const struct oe_sockaddr* sa,
        oe_socklen_t salen,
        char* host,
        oe_socklen_t hostlen,
        char* serv,
        oe_socklen_t servlen,
        int flags);

    // 2Do:     gethostbyaddr(3), getservbyname(3), getservbyport(3),

} oe_resolver_ops_t;

// Well known resolver ids
static const int OE_RESOLVER_ENCLAVE_LOCAL = 0;
static const int OE_RESOLVER_ENCLAVE_DNS = 1; //
static const int OE_RESOLVER_HOST = 2;

typedef struct _oe_resolver
{
    /* Type of this device: OE_DEVICE_ID_FILE or OE_DEVICE_ID_SOCKET. */
    int type;

    /* sizeof additional data. To get a pointer to the device private data,
     * ptr = (oe_file_device_t)(devptr+1); usually sizeof(oe_file_t) or
     * sizeof(oe_socket_t).
     */
    size_t size;

    oe_resolver_ops_t* ops;

} oe_resolver_t;

OE_EXTERNC_END

#endif
