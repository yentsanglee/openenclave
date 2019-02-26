/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#ifndef _OE_LIBCEX_SOCKET_H
#define _OE_LIBCEX_SOCKET_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/device.h>
#include <openenclave/corelibc/arpa/inet.h>
#include <openenclave/corelibc/netdb.h>
#include <openenclave/corelibc/netinet/in.h>
#include <openenclave/corelibc/sys/ioctl.h>
#include <openenclave/corelibc/sys/select.h>
#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/corelibc/unistd.h>
#include <openenclave/libcex/sal.h>

OE_EXTERNC_BEGIN

#define OE_SOCKET_ERROR -1
#define OE_INVALID_SOCKET (oe_socket_t)(~0)
#define OE_IPV6_V6ONLY 27
#define OE_IPPROTO_IPV6 41
#define OE_IPPROTO_TCP 6
#define OE_MSG_WAITALL 0x8
#define OE_TCP_NODELAY 1
#define OE_TCP_KEEPALIVE 3
#define OE_SOMAXCONN 0x7fffffff
#define OE_IOCPARM_MASK 0x7f
#define OE_IOC_IN 0x80000000
#define OE_IOW(x, y, t) \
    (OE_IOC_IN | (((long)sizeof(t) & OE_IOCPARM_MASK) << 16) | ((x) << 8) | (y))
#define OE_FIONBIO OE_IOW('f', 126, u_long)

typedef int oe_socket_t;

typedef enum
{
    OE_NETWORK_INSECURE = 0,
    OE_NETWORK_SECURE_HARDWARE = 1
} oe_network_security_t;

OE_INLINE uint64_t oe_network_security_to_devid(oe_network_security_t security)
{
    switch (security)
    {
        case OE_NETWORK_INSECURE:
            return OE_DEVID_HOST_SOCKET;
        case OE_NETWORK_SECURE_HARDWARE:
            return OE_DEVID_HARDWARE_SECURE_SOCKET;
        default:
            return 0;
    }
}

OE_INLINE int oe_closesocket(_In_ oe_socket_t s)
{
    return oe_close(s);
}

OE_INLINE int __OE_FD_ISSET(int fd, oe_fd_set* set)
{
    return OE_FD_ISSET(fd, set);
}

/*
**==============================================================================
**
** getaddrinfo()
**
**==============================================================================
*/

OE_INLINE int oe_getaddrinfo_OE_NETWORK_INSECURE(
    _In_z_ const char* node,
    _In_z_ const char* service,
    _In_ const struct oe_addrinfo* hints,
    _Out_ struct oe_addrinfo** res)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_getaddrinfo_d(devid, node, service, hints, res);
}

OE_INLINE int oe_getaddrinfo_OE_SECURE_HARDWARE(
    _In_z_ const char* node,
    _In_z_ const char* service,
    _In_ const struct oe_addrinfo* hints,
    _Out_ struct oe_addrinfo** res)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_getaddrinfo_d(devid, node, service, hints, res);
}

OE_INLINE int __oe_getaddrinfo(
    _In_z_ const char* node,
    _In_z_ const char* service,
    _In_ const struct oe_addrinfo* hints,
    _Out_ struct oe_addrinfo** res)
{
#if defined(OE_SECURE_POSIX_NETWORK_API)
    return oe_getaddrinfo_OE_SECURE_HARDWARE(node, service, hints, res);
#else
    return oe_getaddrinfo_OE_NETWORK_INSECURE(node, service, hints, res);
#endif
}

/*
**==============================================================================
**
** gethostname()
**
**==============================================================================
*/

OE_INLINE int oe_gethostname_OE_NETWORK_INSECURE(
    _Out_writes_(len) char* name,
    _In_ size_t len)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_gethostname_d(devid, name, len);
}

OE_INLINE int oe_gethostname_OE_SECURE_HARDWARE(
    _Out_writes_(len) char* name,
    _In_ size_t len)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_gethostname_d(devid, name, len);
}

OE_INLINE int __oe_gethostname(_Out_writes_(len) char* name, _In_ size_t len)
{
#if defined(OE_SECURE_POSIX_NETWORK_API)
    return oe_gethostname_OE_SECURE_HARDWARE(name, len);
#else
    return oe_gethostname_OE_NETWORK_INSECURE(name, len);
#endif
}

/*
**==============================================================================
**
** getnameinfo()
**
**==============================================================================
*/

OE_INLINE int oe_getnameinfo_OE_NETWORK_INSECURE(
    _In_ const struct oe_sockaddr* sa,
    _In_ socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_getnameinfo_d(
        devid, sa, salen, host, hostlen, serv, servlen, flags);
}

OE_INLINE int oe_getnameinfo_OE_NETWORK_SECURE_HARDWARE(
    _In_ const struct oe_sockaddr* sa,
    _In_ socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_getnameinfo_d(
        devid, sa, salen, host, hostlen, serv, servlen, flags);
}

OE_INLINE int __oe_getnameinfo(
    _In_ const struct oe_sockaddr* sa,
    _In_ socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_getnameinfo_OE_NETWORK_SECURE_HARDWARE(
        sa, salen, host, hostlen, serv, servlen, flags);
#else
    return oe_getnameinfo_OE_NETWORK_INSECURE(
        sa, salen, host, hostlen, serv, servlen, flags);
#endif
}

/*
**==============================================================================
**
** socket()
**
**==============================================================================
*/

OE_INLINE oe_socket_t
oe_socket_OE_NETWORK_INSECURE(_In_ int domain, _In_ int type, _In_ int protocol)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_socket_d(devid, domain, type, protocol);
}

OE_INLINE oe_socket_t oe_socket_OE_NETWORK_SECURE_HARDWARE(
    _In_ int domain,
    _In_ int type,
    _In_ int protocol)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_socket_d(devid, domain, type, protocol);
}

OE_INLINE oe_socket_t
__oe_socket(_In_ int domain, _In_ int type, _In_ int protocol)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_socket_OE_NETWORK_SECURE_HARDWARE(domain, type, protocol);
#else
    return oe_socket_OE_NETWORK_INSECURE(domain, type, protocol);
#endif
}

/*
**==============================================================================
**
** oe_wsa_startup()
**
**==============================================================================
*/

typedef struct _oe_wsa_data
{
    int unused;
} oe_wsa_data_t;

OE_INLINE int oe_wsa_startup_OE_NETWORK_INSECURE(
    _In_ uint16_t version_required,
    _Out_ oe_wsa_data_t* wsa_data)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_network_startup_d(devid, version_required, wsa_data);
}

OE_INLINE int oe_wsa_startup_OE_NETWORK_SECURE_HARDWARE(
    _In_ uint16_t version_required,
    _Out_ oe_wsa_data_t* wsa_data)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_network_startup_d(devid, version_required, wsa_data);
}

OE_INLINE int oe_wsa_startup(
    _In_ uint16_t version_required,
    _Out_ oe_wsa_data_t* wsa_data)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_wsa_startup_OE_NETWORK_SECURE_HARDWARE(
        version_required, wsa_data);
#else
    return oe_wsa_startup_OE_NETWORK_INSECURE(version_required, wsa_data);
#endif
}

/*
**==============================================================================
**
** oe_wsa_cleanup()
**
**==============================================================================
*/

OE_INLINE int oe_wsa_cleanup_OE_NETWORK_INSECURE(void)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_network_shutdown_d(devid);
}

OE_INLINE int oe_wsa_cleanup_OE_NETWORK_SECURE_HARDWARE(void)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_network_shutdown_d(devid);
}

OE_INLINE int oe_wsa_cleanup(void)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_wsa_cleanup_OE_NETWORK_SECURE_HARDWARE();
#else
    return oe_wsa_cleanup_OE_NETWORK_INSECURE();
#endif
}

/*
**==============================================================================
**
** oe_wsa_get_last_error()
**
**==============================================================================
*/

OE_INLINE int oe_wsa_get_last_error_OE_NETWORK_INSECURE(void)
{
    uint64_t devid = OE_DEVID_HOST_SOCKET;
    return oe_network_errno_d(devid);
}

OE_INLINE int oe_wsa_get_last_error_OE_NETWORK_SECURE_HARDWARE(void)
{
    uint64_t devid = OE_DEVID_HARDWARE_SECURE_SOCKET;
    return oe_network_errno_d(devid);
}

OE_INLINE int oe_wsa_get_last_error(void)
{
#ifdef OE_SECURE_POSIX_NETWORK_API
    return oe_wsa_get_last_error_OE_NETWORK_SECURE_HARDWARE();
#else
    return oe_wsa_get_last_error_OE_NETWORK_INSECURE();
#endif
}

/*
**==============================================================================
**
** Standard names:
**
**==============================================================================
*/

#if !defined(OE_NO_POSIX_SOCKET_API)
#define getaddrinfo __oe_getaddrinfo
#define gethostname __oe_gethostname
#define getnameinfo __oe_getnameinfo
#define socket __oe_socket
#define accept oe_accept
#define addrinfo oe_addrinfo
#define AI_PASSIVE OE_AI_PASSIVE
#define AI_CANONNAME OE_AI_CANONNAME
#define AI_NUMERICHOST OE_AI_NUMERICHOST
#define AI_ALL OE_AI_ALL
#define AI_ADDRCONFIG OE_AI_ADDRCONFIG
#define AI_V4MAPPED OE_AI_V4MAPPED
#define AF_INET OE_AF_INET
#define AF_INET6 OE_AF_INET6
#define bind oe_bind
#define connect oe_connect
// ATTN:IO!!!
//#define FD_ISSET(fd, set) __OE_FD_ISSET((oe_socket_t)(fd), (oe_fd_set*)(set))
#define fd_set oe_fd_set
#define FIONBIO OE_FIONBIO
#define freeaddrinfo oe_freeaddrinfo
#define getpeername oe_getpeername
#define getsockname oe_getsockname
#define getsockopt oe_getsockopt
#define htonl oe_htonl
#define htons oe_htons
#define in_addr oe_in_addr
#define in6_addr oe_in6_addr
#define inet_addr oe_inet_addr
#define INVALID_SOCKET OE_INVALID_SOCKET
#define IPV6_V6ONLY OE_IPV6_V6ONLY
#define IPPROTO_IPV6 OE_IPPROTO_IPV6
#define IPPROTO_TCP OE_IPPROTO_TCP
#define listen oe_listen
#define MSG_WAITALL OE_MSG_WAITALL
#define NI_NOFQDN OE_NI_NOFQDN
#define NI_NUMERICHOST OE_NI_NUMERICHOST
#define NI_NAMEREQD OE_NI_NAMEREQD
#define NI_NUMERICSERV OE_NI_NUMERICSERV
#define NI_DGRAM OE_NI_DGRAM
#define NI_MAXHOST OE_NI_MAXHOST
#define NI_MAXSERV OE_NI_MAXSERV
#define ntohl oe_ntohl
#define ntohs oe_ntohs
#define recv oe_recv
#define select oe_select
#define send oe_send
#define setsockopt oe_setsockopt
#define shutdown oe_shutdown
#define SOCK_STREAM OE_SOCK_STREAM
#define sockaddr oe_sockaddr
#define sockaddr_in oe_sockaddr_in
#define sockaddr_in6 oe_sockaddr_in6
#define sockaddr_storage oe_sockaddr_storage
#define socklen_t oe_socklen_t
#define SOCKET oe_socket_t
#define SOL_SOCKET OE_SOL_SOCKET
#define SO_ERROR OE_SO_ERROR
#define SO_KEEPALIVE OE_SO_KEEPALIVE
#define SO_RCVBUF OE_SO_RCVBUF
#define SO_SNDBUF OE_SO_SNDBUF
#define SOMAXCONN OE_SOMAXCONN
#define TCP_KEEPALIVE OE_TCP_KEEPALIVE
#define TCP_NODELAY OE_TCP_NODELAY
#endif

#ifndef OE_NO_WINSOCK_API
#define closesocket oe_closesocket
#define ioctlsocket oe_ioctl
#define SOCKET_ERROR OE_SOCKET_ERROR
#define WSADATA oe_wsa_data_t
#define WSAECONNABORTED OE_ECONNABORTED
#define WSAECONNRESET OE_ECONNRESET
#define WSAEINPROGRESS OE_EINPROGRESS
#define WSAEWOULDBLOCK OE_EAGAIN
#define WSAStartup oe_wsa_startup
#define WSACleanup oe_wsa_cleanup
#define WSASetLastError oe_wsa_get_last_error
#endif

OE_EXTERNC_END

#endif /* _OE_LIBCEX_SOCKET_H */
