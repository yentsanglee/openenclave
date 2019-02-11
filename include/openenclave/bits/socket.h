/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_BITS_SOCKET_H
#define _OE_BITS_SOCKET_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/in.h>
#include <openenclave/bits/sockaddr.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/* oe_setsockopt()/oe_getsockopt() options. */
#define OE_SOL_SOCKET 1
#define OE_SO_DEBUG 1
#define OE_SO_REUSEADDR 2
#define OE_SO_TYPE 3
#define OE_SO_ERROR 4
#define OE_SO_DONTROUTE 5
#define OE_SO_BROADCAST 6
#define OE_SO_SNDBUF 7
#define OE_SO_RCVBUF 8
#define OE_SO_SNDBUFFORCE 32
#define OE_SO_RCVBUFFORCE 33
#define OE_SO_KEEPALIVE 9
#define OE_SO_OOBINLINE 10
#define OE_SO_NO_CHECK 11
#define OE_SO_PRIORITY 12
#define OE_SO_LINGER 13
#define OE_SO_BSDCOMPAT 14
#define OE_SO_REUSEPORT 15

/* oe_shutdown() options. */
#define OE_SHUT_RD 0
#define OE_SHUT_WR 1
#define OE_SHUT_RDWR 2

typedef uint32_t oe_socklen_t;

int oe_socket(int domain, int type, int protocol);

int oe_accept(int s, struct oe_sockaddr* addr, oe_socklen_t* addrlen);

int oe_bind(int s, const oe_sockaddr* name, oe_socklen_t namelen);

int oe_connect(int s, const oe_sockaddr* name, oe_socklen_t namelen);

uint32_t oe_htonl(uint32_t hostLong);

uint16_t oe_htons(uint16_t hostShort);

uint32_t oe_inet_addr(const char* cp);

int oe_listen(int s, int backlog);

uint32_t oe_ntohl(uint32_t netLong);

uint16_t oe_ntohs(uint16_t netShort);

ssize_t oe_recv(int s, void* buf, size_t len, int flags);

ssize_t oe_send(int s, const void* buf, size_t len, int flags);

int oe_setsockopt(
    int s,
    int level,
    int optname,
    const void* optval,
    oe_socklen_t optlen);

int oe_shutdown(int s, int how);

OE_EXTERNC_END

#endif /* _OE_BITS_SOCKET_H */
