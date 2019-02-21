// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <netinet/in.h>
#include <sys/socket.h>

#include <openenclave/corelibc/netinet/in.h>
#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/internal/defs.h>

/*
**==============================================================================
**
** Verify that oe_sockaddr and sockaddr have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(sizeof(struct oe_sockaddr) == sizeof(struct sockaddr));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr, sa_family) ==
    OE_OFFSETOF(struct sockaddr, sa_family));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr, sa_data) ==
    OE_OFFSETOF(struct sockaddr, sa_data));

/*
**==============================================================================
**
** Verify that oe_sockaddr_storage and sockaddr_storage have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(
    sizeof(struct oe_sockaddr_storage) == sizeof(struct sockaddr_storage));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_storage, ss_family) ==
    OE_OFFSETOF(struct sockaddr_storage, ss_family));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_storage, __ss_padding) ==
    OE_OFFSETOF(struct sockaddr_storage, __ss_padding));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_storage, __ss_align) ==
    OE_OFFSETOF(struct sockaddr_storage, __ss_align));

/*
**==============================================================================
**
** Verify that oe_in_addr and in_addr have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(sizeof(struct oe_in_addr) == sizeof(struct in_addr));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_in_addr, s_addr) ==
    OE_OFFSETOF(struct in_addr, s_addr));

/*
**==============================================================================
**
** Verify that oe_in6_addr and in6_addr have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(sizeof(struct oe_in6_addr) == sizeof(struct in6_addr));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_in6_addr, __in6_union) ==
    OE_OFFSETOF(struct in6_addr, __in6_union));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_in6_addr, oe_s6_addr) ==
    OE_OFFSETOF(struct in6_addr, s6_addr));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_in6_addr, oe_s6_addr16) ==
    OE_OFFSETOF(struct in6_addr, oe_s6_addr16));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_in6_addr, oe_s6_addr32) ==
    OE_OFFSETOF(struct in6_addr, oe_s6_addr32));

/*
**==============================================================================
**
** Verify that oe_sockaddr_in and sockaddr_in have same layout.
**
**==============================================================================
*/

OE_STATIC_ASSERT(sizeof(struct oe_sockaddr_in) == sizeof(struct sockaddr_in));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_in, sin_family) ==
    OE_OFFSETOF(struct sockaddr_in, sin_family));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_in, sin_port) ==
    OE_OFFSETOF(struct sockaddr_in, sin_port));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_in, sin_addr) ==
    OE_OFFSETOF(struct sockaddr_in, sin_addr));

OE_STATIC_ASSERT(
    OE_OFFSETOF(struct oe_sockaddr_in, sin_zero) ==
    OE_OFFSETOF(struct sockaddr_in, sin_zero));
