// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
**==============================================================================
**
** host_socket.h
**
**     Definition of the host_socket internal types and data
**
**==============================================================================
*/

#ifndef _OE_SOCKETADDR_H__
#define _OE_SOCKETADDR_H__

OE_EXTERNC_BEGIN

#ifndef _SS_MAXSIZE
#define _SS_MAXSIZE 128
#define _SS_ALIGNSIZE (sizeof(int64_t))
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof(oe_sa_family_t))
#define _SS_PAD2SIZE \
    (_SS_MAXSIZE - (sizeof(oe_sa_family_t) + _SS_PAD1SIZE + _SS_ALIGNSIZE))
#endif
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 65
#endif

typedef unsigned short int oe_sa_family_t;

typedef struct oe_sockaddr_storage
{
    oe_sa_family_t ss_family;
    char __ss_pad1[_SS_PAD1SIZE];
    int64_t __ss_align;
    char __ss_pad2[_SS_PAD2SIZE];
} oe_sockaddr_storage;

typedef struct oe_in_addr
{
    uint32_t s_addr;
} oe_in_addr;

#ifndef INADDR_ANY
#define INADDR_ANY 0x00000000
#endif
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a)                            \
    (((a)->s6_words[0] == 0) && ((a)->s6_words[1] == 0) && \
     ((a)->s6_words[2] == 0) && ((a)->s6_words[3] == 0) && \
     ((a)->s6_words[4] == 0) && ((a)->s6_words[5] == 0xffff))
#endif

typedef struct oe_sockaddr_in
{
    oe_sa_family_t sin_family;
    uint16_t sin_port;
    oe_in_addr sin_addr;
} oe_sockaddr_in;

typedef struct oe_in6_addr
{
    union {
        uint8_t Byte[16];
        uint16_t Word[8];
    } u;
} oe_in6_addr;

#ifndef s6_addr
#define s6_addr u.Byte
#endif
#ifndef s6_words
#define s6_words u.Word
#endif

#ifndef IN6ADDR_LOOPBACK_INIT
#define IN6ADDR_LOOPBACK_INIT                          \
    {                                                  \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 \
    }
#endif

typedef struct oe_sockaddr_in6
{
    oe_sa_family_t sin6_family;
    uint16_t sin6_port;
    uint32_t sin6_flowinfo;
    oe_in6_addr sin6_addr;
    uint32_t sin6_scope_id;
} oe_sockaddr_in6;

typedef uint16_t oe_sa_family_t;
typedef struct oe_sockaddr
{
    oe_sa_family_t sa_family;
    char sa_data[14];
} oe_sockaddr;

OE_EXTERNC_END

#endif /* _OE_SOCKETADDR_H */
