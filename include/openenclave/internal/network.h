// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_NETWORK_H
#define _OE_NETWORK_H

#include <openenclave/bits/types.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/device.h>

OE_EXTERNC_BEGIN

typedef uint32_t socklen_t;
struct oe_sockaddr;
struct oe_addrinfo;
typedef struct _oe_socket oe_socket_t;
typedef struct _oe_network oe_network_t;

struct _oe_socket
{
    oe_device_t device;

    int (*connect)(
        oe_socket_t* sock,
        const struct oe_sockaddr *addr,
        socklen_t addrlen);

    oe_socket_t* (*accept)(
        oe_socket_t* sock,
        struct oe_sockaddr *addr,
        socklen_t *addrlen);

    int (*bind)(
        oe_socket_t* sock,
        const struct oe_sockaddr *addr,
        socklen_t addrlen);

    int (*listen)(oe_socket_t* sock, int backlog);

    ssize_t (*recv)(oe_socket_t* sock, void *buf, size_t len, int flags);

    ssize_t (*send)(oe_socket_t* sock, const void *buf, size_t len, int flags);

    int (*shutdown)(oe_socket_t* sock, int how);

    int (*getsockopt)(
        oe_socket_t* sock,
        int level,
        int optname,
        void* optval,
        socklen_t* optlen);

    int (*setsockopt)(
        oe_socket_t* sock,
        int level,
        int optname,
        const void* optval,
        socklen_t optlen);

    int (*getpeername)(
        oe_socket_t* sock, struct oe_sockaddr *addr, socklen_t *addrlen);

    int (*getsockname)(oe_socket_t* sock, struct oe_sockaddr *addr, socklen_t *addrlen);
};

struct _oe_network
{
    /* Decrement the internal reference count and release if zero. */
    void (*release)(oe_fs_t* fs);

    /* Increment the internal reference count. */
    void (*add_ref)(oe_fs_t* fs);

    /* Socket factory method. */
    oe_socket_t* (*socket)(oe_network_t* network, int domain, int type, int protocol);

    int (*getaddrinfo)(
        oe_network_t* network,
        const char *node,
        const char *service,
        const struct oe_addrinfo *hints,
        struct oe_addrinfo **res);

    void (*freeaddrinfo)(
        oe_network_t* network,
        struct oe_addrinfo *res);

    int (*gethostname)(
        oe_network_t* network,
        char *name, size_t len);

    int (*getnameinfo)(
        oe_network_t* network,
        const struct oe_sockaddr *sa,
        socklen_t salen,
        char *host,
        socklen_t hostlen,
        char *serv,
        socklen_t servlen,
        int flags);
};

/* ATTN: where does select go? */

OE_EXTERNC_END

#endif // _OE_NETWORK_H
