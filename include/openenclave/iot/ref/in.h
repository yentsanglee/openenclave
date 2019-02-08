// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef __NETINET_IN_H__
#define __NETINET_IN_H__

#include <openenclave/internal/sockaddr.h>
OE_EXTERNC_BEGIN

/* Internet address.  */
typedef uint32_t oe_in_addr_t;
struct oe_in_addr
{
    oe_in_addr_t s_addr;
};

/* Type to represent a port.  */
typedef uint16_t oe_in_port_t;

/* Address to accept any incoming messages.  */
#define OE_INADDR_ANY ((oe_in_addr_t)0x00000000)
/* Address to send to all hosts.  */
#define OE_INADDR_BROADCAST ((oe_in_addr_t)0xffffffff)
/* Address indicating an error return.  */
#define OE_INADDR_NONE ((oe_in_addr_t)0xffffffff)

/* Address to loopback in software to local host.  */
#ifndef OE_INADDR_LOOPBACK
#define OE_INADDR_LOOPBACK ((oe_in_addr_t)0x7f000001) /* Inet 127.0.0.1.  */
#endif

struct oe_in6_addr
{
    union {
        uint8_t __u6_addr8[16];
        uint16_t __u6_addr16[8];
        uint32_t __u6_addr32[4];
    } __in6_u;
#define s6_addr __in6_u.__u6_addr8
#ifdef __USE_MISC
#define s6_addr16 __in6_u.__u6_addr16
#define s6_addr32 __in6_u.__u6_addr32
#endif
};

extern const struct oe_in6_addr in6addr_any;      /* :: */
extern const struct oe_in6_addr in6addr_loopback; /* ::1 */
#define OE_IN6ADDR_ANY_INIT                                    \
    {                                                          \
        {                                                      \
            {                                                  \
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
            }                                                  \
        }                                                      \
    }
#define OE_IN6ADDR_LOOPBACK_INIT                               \
    {                                                          \
        {                                                      \
            {                                                  \
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 \
            }                                                  \
        }                                                      \
    }

#define OE_INET_ADDRSTRLEN 16
#define OE_INET6_ADDRSTRLEN 46

struct oe_sockaddr_in
{
    oe_sa_family_t sin_family;
    oe_in_port_t sin_port;      /* Port number.  */
    struct oe_in_addr sin_addr; /* Internet address.  */

    /* Pad to size of `struct sockaddr'.  */
    unsigned char sin_zero
        [sizeof(struct oe_sockaddr) - __SOCKADDR_COMMON_SIZE -
         sizeof(oe_in_port_t) - sizeof(struct oe_in_addr)];
};

#include <endian.h>
#if __OE_BYTE_ORDER == __OE_BIG_ENDIAN
__inline uint32_t oe_ntohl(uint32_t __netlong)
{
    return __netlong;
}
__inline uint16_t oe_ntohs(uint16_t __netshort)
{
    return __netshort;
}
__inline uint32_t oe_htonl(uint32_t __hostlong)
{
    return __hostlong;
}
__inline uint16_t oe_htons(uint16_t __hostshort)
{
    return __hostshort;
}
#else
#if defined(MSVC)
__inline uint32_t oe_ntohl(uint32_t __netlong)
{
    return __byteswap_ulong(__netlong);
}
__inline uint16_t oe_ntohs(uint16_t __netshort)
{
    return __byteswap_ushort(__netshort);
}
__inline uint32_t oe_htonl(uint32_t __hostlong)
{
    return __byteswap_ulong(__hostlong);
}
__inline uint16_t oe_htons(uint16_t __hostshort)
{
    return __byteswap_ushort(__hostshort);
}
#else
__inline uint32_t oe_ntohl(uint32_t __netlong)
{
    return __builtin_bswap32(__netlong);
}
__inline uint16_t oe_ntohs(uint16_t __netshort)
{
    return __builtin_bswap16(__netshort);
}
__inline uint32_t oe_htonl(uint32_t __hostlong)
{
    return __builtin_bswap32(__hostlong);
}
__inline uint16_t oe_htons(uint16_t __hostshort)
{
    return __builtin_bswap16(__hostshort);
}
#endif
#endif

OE_EXTERNC_END

#endif /* netinet/in.h */
