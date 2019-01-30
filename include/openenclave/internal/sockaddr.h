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

#define __SOCKADDR_COMMON(sa_prefix) oe_sa_family_t sa_prefix##family

#define __SOCKADDR_COMMON_SIZE (sizeof(unsigned short int))

#define _SS_SIZE 128

typedef unsigned short int oe_sa_family_t;

typedef struct oe_sockaddr_storage
{
    oe_sa_family_t ss_family;
    char __ss_pad1[_SS_PAD1SIZE];
    int64_t __ss_align;
    char __ss_pad2[_SS_PAD2SIZE];
} oe_sockaddr_storage;


typedef uint16_t oe_sa_family_t;
typedef struct oe_sockaddr
{
    oe_sa_family_t sa_family;
    char sa_data[14];
} oe_sockaddr;

OE_EXTERNC_END

#endif /* _OE_SOCKETADDR_H */
