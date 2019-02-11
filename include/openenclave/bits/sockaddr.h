/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_BITS_SOCKADDR_H
#define _OE_BITS_SOCKADDR_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/in.h>
#include <openenclave/bits/types.h>

OE_EXTERNC_BEGIN

/*
**==============================================================================
**
** Generic socket address definitions:
**
**==============================================================================
*/

/* Address to accept any incoming messages. */
#define OE_INADDR_ANY ((oe_in_addr_t)0x00000000)

/* Address to send to all hosts. */
#define OE_INADDR_BROADCAST ((oe_in_addr_t)0xffffffff)

/* Address indicating an error return. */
#define OE_INADDR_NONE ((oe_in_addr_t)0xffffffff)

/* Address to loopback in software to local host. */
#define OE_INADDR_LOOPBACK ((oe_in_addr_t)0x7f000001) /* Inet 127.0.0.1.  */

/*
**==============================================================================
**
** struct oe_in_addr:
**
**==============================================================================
*/

#define OE_INET_ADDRSTRLEN 16

typedef uint32_t oe_in_addr_t;

struct oe_in_addr
{
    oe_in_addr_t s_addr;
};

/*
**==============================================================================
**
** struct oe_in6_addr:
**
**==============================================================================
*/

#define OE_INET6_ADDRSTRLEN 46

struct oe_in6_addr
{
    union {
        uint8_t __u6_addr8[16];
        uint16_t __u6_addr16[8];
        uint32_t __u6_addr32[4];
    } __in6_u;
};

#define s6_addr __in6_u.__u6_addr8
#define s6_addr16 __in6_u.__u6_addr16
#define s6_addr32 __in6_u.__u6_addr32

extern const struct oe_in6_addr in6addr_any;      /* :: */
extern const struct oe_in6_addr in6addr_loopback; /* ::1 */

// clang-format off
#define OE_IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define OE_IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }
// clang-format on

/*
**==============================================================================
**
** struct oe_sockaddr:
**
**==============================================================================
*/

typedef uint16_t oe_sa_family_t;

typedef struct oe_sockaddr
{
    oe_sa_family_t sa_family;
    char sa_data[14];
} oe_sockaddr;

/*
**==============================================================================
**
** struct oe_sockaddr_storage:
**
**==============================================================================
*/

typedef unsigned short int oe_sa_family_t;

struct oe_sockaddr_storage
{
    oe_sa_family_t ss_family;
    char __ss_padding[128 - sizeof(long) - sizeof(oe_sa_family_t)];
    unsigned long __ss_align;
};

/*
**==============================================================================
**
** struct oe_sockaddr_in:
**
**==============================================================================
*/

typedef uint16_t oe_in_port_t;

struct oe_sockaddr_in
{
    oe_sa_family_t sin_family;
    oe_in_port_t sin_port;
    struct oe_in_addr sin_addr;
    uint8_t sin_zero[8];
};

/*
**==============================================================================
**
** OE_AF_INET
** OE_AF_INET6
**
**==============================================================================
*/

#define OE_AF_INET 2
#define OE_AF_ENCLAVE 50

#ifdef __linux__
#define OE_AF_INET6 10
#else
/* ATTN: which platform is this for? */
#define OE_AF_INET6 23
#endif

/*
**==============================================================================
**
** Supported address families:
**
**==============================================================================
*/

#define OE_SOCK_STREAM 1

OE_EXTERNC_END

#endif /* _OE_BITS_SOCKADDR_H */
