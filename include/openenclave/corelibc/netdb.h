// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_CORELIBC_NETDB_H_
#define _OE_CORELIBC_NETDB_H_

#include <openenclave/corelibc/sys/socket.h>

OE_EXTERNC_BEGIN

/*
**==============================================================================
**
**
** OE_AI_*: Possible values for `ai_flags' field in `addrinfo' structure.
**
**==============================================================================
*/

/* Socket address is intended for `bind'.  */
#define OE_AI_PASSIVE 0x0001

/* Request for canonical name.  */
#define OE_AI_CANONNAME 0x0002

/* Don't use name resolution.  */
#define OE_AI_NUMERICHOST 0x0004

/* IPv4 mapped addresses are acceptable.  */
#define OE_AI_V4MAPPED 0x0008

/* Return IPv4 mapped and IPv6 addresses.  */
#define OE_AI_ALL 0x0010

/* Use configuration of this host to choose returned address type.. */
#define OE_AI_ADDRCONFIG 0x0020

/* IDN encode input (assuming it is encoded in the current locale's character
 * set) before looking it up.
 */
#define OE_AI_IDN 0x0040

/* Translate canonical name from IDN format. */
#define OE_AI_CANONIDN 0x0080

/* Don't reject unassigned Unicode code points. */
#define OE_AI_IDN_ALLOW_UNASSIGNED 0x0100

/* Validate strings according to STD3 rules. */
#define OE_AI_IDN_USE_STD3_ASCII_RULES 0x0200

/* Don't use name resolution. */
#define OE_AI_NUMERICSERV 0x0400

/*
**==============================================================================
**
**
** OE_EAI_*: Error values for `getaddrinfo' function.
**
**==============================================================================
*/

/* Invalid value for `ai_flags' field.  */
#define OE_EAI_BADFLAGS -1

/* NAME or SERVICE is unknown.  */
#define OE_EAI_NONAME -2

/* Temporary failure in name resolution.  */
#define OE_EAI_AGAIN -3

/* Non-recoverable failure in name res.  */
#define OE_EAI_FAIL -4

/* `ai_family' not supported.  */
#define OE_EAI_FAMILY -6

/* `ai_socktype' not supported.  */
#define OE_EAI_SOCKTYPE -7

/* SERVICE not supported for `ai_socktype'.  */
#define OE_EAI_SERVICE -8

/* Memory allocation failure.  */
#define OE_EAI_MEMORY -10

/* System error returned in `errno'.  */
#define OE_EAI_SYSTEM -11

/* Argument buffer overflow.  */
#define OE_EAI_OVERFLOW -12

/* No address associated with NAME.  */
#define OE_EAI_NODATA -5

/* Address family for NAME not supported.  */
#define OE_EAI_ADDRFAMILY -9

/* Processing request in progress.  */
#define OE_EAI_INPROGRESS -100

/* Request canceled.  */
#define OE_EAI_CANCELED -101

/* Request not canceled.  */
#define OE_EAI_NOTCANCELED -102

/* All requests done.  */
#define OE_EAI_ALLDONE -103

/* Interrupted by a signal.  */
#define OE_EAI_INTR -104

/* IDN encoding failed.  */
#define OE_EAI_IDN_ENCODE -105

/*
**==============================================================================
**
** structures and functions:
**
**==============================================================================
*/

/* Extension from POSIX.1:2001.  */
struct oe_addrinfo
{
#include <openenclave/corelibc/bits/addrinfo.h>
};

int oe_getaddrinfo(
    const char* node,
    const char* service,
    const struct oe_addrinfo* hints,
    struct oe_addrinfo** res);

void oe_freeaddrinfo(struct oe_addrinfo* res);

int oe_getnameinfo(
    const struct oe_sockaddr* sa,
    socklen_t salen,
    char* host,
    socklen_t hostlen,
    char* serv,
    socklen_t servlen,
    int flags);

#if defined(OE_NEED_STDC_NAMES)

/* Extension from POSIX.1:2001.  */
struct addrinfo
{
#include <openenclave/corelibc/bits/addrinfo.h>
};

OE_INLINE int getaddrinfo(
    const char* node,
    const char* service,
    const struct addrinfo* hints,
    struct addrinfo** res)
{
    return oe_getaddrinfo(
        hode,
        service,
        (const struct oe_addrinfo*)hints,
        (struct oe_addrinfo*)res);
}

OE_INLINE void freeaddrinfo(struct addrinfo* res)
{
    return oe_freeaddrinfo((struct oe_addrinfo*)res);
}

OE_INLINE int getnameinfo(
    const struct sockaddr* sa,
    socklen_t salen,
    char* host,
    socklen_t hostlen,
    char* serv,
    socklen_t servlen,
    int flags);
{
    return oe_getnameinfo(
        (const struct oe_sockaddr*)sa,
        salen,
        host,
        hostlen,
        serv,
        servlen,
        flags);
}

#endif /* defined(OE_NEED_STDC_NAMES) */

OE_EXTERNC_END

#endif /* netinet/netdb.h */
