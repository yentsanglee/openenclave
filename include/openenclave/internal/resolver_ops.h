// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef __OE_RESOLVER_H__
#define __OE_RESOLVER_H__

#include "sock_ops.h"

typedef struct _oe_resolver
{
    int (*getaddrinfo)(
        oe_device_t* dev,
        const char* node,
        const char* service,
        const struct oe_addrinfo* hints,
        struct oe_addrinfo** res);

    void (*freeaddrinfo)(oe_device_t* dev, struct oe_addrinfo* res);
    int (*gethostname)(oe_device_t* dev, char* name, size_t len);

    int (*getnameinfo)(
        oe_device_t* dev,
        const struct oe_sockaddr* sa,
        oe_socklen_t salen,
        char* host,
        oe_socklen_t hostlen,
        char* serv,
        oe_socklen_t servlen,
        int flags);
} oe_resolver_t;

#endif
