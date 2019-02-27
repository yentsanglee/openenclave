/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_LIBCEX_SOCKET_H
#define _OE_LIBCEX_SOCKET_H

#include <netdb.h>
#include <netinet/in.h>
#include <openenclave/bits/defs.h>
#include <openenclave/corelibc/netdb.h>
#include <openenclave/corelibc/unistd.h>
#include <openenclave/libcex/sal.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openenclave/corelibc/sys/ioctl.h>

OE_EXTERNC_BEGIN

OE_INLINE int oe_closesocket(_In_ int s)
{
    return oe_close(s);
}

OE_INLINE int oe_ioctlsocket(int fd, unsigned long request, ...)
{
    oe_va_list ap;
    oe_va_start(ap, request);
    int r = oe_ioctl_va(fd, request, ap);
    oe_va_end(ap);
    return r;
}

OE_INLINE int oe_getaddrinfo_insecure(
    _In_z_ const char* node,
    _In_z_ const char* service,
    _In_ const struct addrinfo* hints,
    _Out_ struct addrinfo** res)
{
    return oe_getaddrinfo_d(
        OE_DEVID_HOST_SOCKET,
        node,
        service,
        (const struct oe_addrinfo*)hints,
        (struct oe_addrinfo**)res);
}

OE_INLINE int oe_getaddrinfo_secure_hardware(
    _In_z_ const char* node,
    _In_z_ const char* service,
    _In_ const struct addrinfo* hints,
    _Out_ struct addrinfo** res)
{
    return oe_getaddrinfo_d(
        OE_DEVID_HARDWARE_SECURE_SOCKET,
        node,
        service,
        (const struct oe_addrinfo*)hints,
        (struct oe_addrinfo**)res);
}

OE_INLINE int oe_getaddrinfo_default(
    _In_z_ const char* node,
    _In_z_ const char* service,
    _In_ const struct addrinfo* hints,
    _Out_ struct addrinfo** res)
{
#if defined(OE_SECURE_POSIX_NETWORK_API)
    return oe_getaddrinfo_secure_hardware(node, service, hints, res);
#else
    return oe_getaddrinfo_insecure(node, service, hints, res);
#endif
}

OE_INLINE int oe_gethostname_insecure(
    _Out_writes_(len) char* name,
    _In_ size_t len)
{
    return oe_gethostname_d(OE_DEVID_HOST_SOCKET, name, len);
}

OE_INLINE int oe_gethostname_secure_hardware(
    _Out_writes_(len) char* name,
    _In_ size_t len)
{
    return oe_gethostname_d(OE_DEVID_HARDWARE_SECURE_SOCKET, name, len);
}

OE_INLINE int oe_gethostname_default(
    _Out_writes_(len) char* name,
    _In_ size_t len)
{
#if defined(OE_SECURE_POSIX_NETWORK_API)
    return oe_gethostname_secure_hardware(name, len);
#else
    return oe_gethostname_insecure(name, len);
#endif
}

OE_INLINE int oe_getnameinfo_insecure(
    _In_ const struct oe_sockaddr* sa,
    _In_ socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
    return oe_getnameinfo_d(
        OE_DEVID_HOST_SOCKET, sa, salen, host, hostlen, serv, servlen, flags);
}

OE_INLINE int oe_getnameinfo_secure_hardware(
    _In_ const struct oe_sockaddr* sa,
    _In_ socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
    return oe_getnameinfo_d(
        OE_DEVID_HARDWARE_SECURE_SOCKET,
        sa,
        salen,
        host,
        hostlen,
        serv,
        servlen,
        flags);
}

OE_INLINE int oe_getnameinfo_default(
    _In_ const struct oe_sockaddr* sa,
    _In_ socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_getnameinfo_secure_hardware(
        sa, salen, host, hostlen, serv, servlen, flags);
#else
    return oe_getnameinfo_insecure(
        sa, salen, host, hostlen, serv, servlen, flags);
#endif
}

OE_INLINE int oe_socket_insecure(
    _In_ int domain,
    _In_ int type,
    _In_ int protocol)
{
    return oe_socket_d(OE_DEVID_HOST_SOCKET, domain, type, protocol);
}

OE_INLINE int oe_socket_secure_hardware(
    _In_ int domain,
    _In_ int type,
    _In_ int protocol)
{
    return oe_socket_d(OE_DEVID_HARDWARE_SECURE_SOCKET, domain, type, protocol);
}

OE_INLINE int oe_socket_default(
    _In_ int domain,
    _In_ int type,
    _In_ int protocol)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_socket_secure_hardware(domain, type, protocol);
#else
    return oe_socket_insecure(domain, type, protocol);
#endif
}

#if !defined(OE_NO_POSIX_SOCKET_API)
#define getaddrinfo oe_getaddrinfo_default
#define getnameinfo oe_getnameinfo_default
#define gethostname oe_gethostname_default
#define socket oe_socket_default
#endif

OE_EXTERNC_END

#endif /* _OE_LIBCEX_SOCKET_H */
