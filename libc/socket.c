// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <netinet/in.h>
#include <sys/socket.h>

#include <openenclave/corelibc/netinet/in.h>
#include <openenclave/corelibc/sys/socket.h>
#include "check_field.h"

OE_STATIC_ASSERT(sizeof(struct oe_sockaddr) == sizeof(struct sockaddr));
CHECK_FIELD(struct oe_sockaddr, struct sockaddr, sa_family);
CHECK_FIELD(struct oe_sockaddr, struct sockaddr, sa_data);

OE_STATIC_ASSERT(
    sizeof(struct oe_sockaddr_storage) == sizeof(struct sockaddr_storage));
CHECK_FIELD(struct oe_sockaddr_storage, struct sockaddr_storage, ss_family);
CHECK_FIELD(struct oe_sockaddr_storage, struct sockaddr_storage, __ss_padding);
CHECK_FIELD(struct oe_sockaddr_storage, struct sockaddr_storage, __ss_align);

OE_STATIC_ASSERT(sizeof(struct oe_in_addr) == sizeof(struct in_addr));
CHECK_FIELD(struct oe_in_addr, struct in_addr, s_addr);

OE_STATIC_ASSERT(sizeof(struct oe_in6_addr) == sizeof(struct in6_addr));
CHECK_FIELD(struct oe_in6_addr, struct in6_addr, __in6_union);
CHECK_FIELD(struct oe_in6_addr, struct in6_addr, oe_s6_addr);
CHECK_FIELD(struct oe_in6_addr, struct in6_addr, oe_s6_addr16);
CHECK_FIELD(struct oe_in6_addr, struct in6_addr, oe_s6_addr32);

OE_STATIC_ASSERT(sizeof(struct oe_sockaddr_in) == sizeof(struct sockaddr_in));
CHECK_FIELD(struct oe_sockaddr_in, struct sockaddr_in, sin_family);
CHECK_FIELD(struct oe_sockaddr_in, struct sockaddr_in, sin_port);
CHECK_FIELD(struct oe_sockaddr_in, struct sockaddr_in, sin_addr);
CHECK_FIELD(struct oe_sockaddr_in, struct sockaddr_in, sin_zero);
