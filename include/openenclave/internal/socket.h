/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#pragma once
#ifndef __OE_SOCKET_H__
#define __OE_SOCKET_H__
#include <openenclave/bits/sockettypes.h>
#include <openenclave/bits/timetypes.h>

#include <openenclave/bits/sal_unsup.h>
#include <openenclave/internal/sockaddr.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef int32_t oe_sockfd_t;
    typedef uint32_t oe_socklen_t;

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

#if 0
#define OE_SOCKET_ERROR -1
#define OE_INVALID_SOCKET (int)(~0)
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

#define OE_AI_PASSIVE 0x00000001
#define OE_AI_CANONNAME 0x00000002
#define OE_AI_NUMERICHOST 0x00000004
#define OE_AI_ALL 0x00000100
#define OE_AI_ADDRCONFIG 0x00000400
#define OE_AI_V4MAPPED 0x00000800

#define OE_NI_NOFQDN 0x01
#define OE_NI_NUMERICHOST 0x02
#define OE_NI_NAMEREQD 0x04
#define OE_NI_NUMERICSERV 0x08
#define OE_NI_DGRAM 0x10
#define OE_NI_MAXHOST 1025
#define OE_NI_MAXSERV 32
#endif

#if defined(OE_MAP_SIX_SOCKET_API)
/* Map standard socket API names to the OE equivalents. */
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
#define FD_ISSET(fd, set) oe_fd_isset((int)(fd), (oe_fd_set*)(set))
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
#define send oe_send
#define setsockopt oe_setsockopt
#define shutdown oe_shutdown
#define SOCK_STREAM OE_SOCK_STREAM
#define sockaddr oe_sockaddr
#define sockaddr_in oe_sockaddr_in
#define sockaddr_in6 oe_sockaddr_in6
#define sockaddr_storage oe_sockaddr_storage
#define SOL_SOCKET OE_SOL_SOCKET
#define SO_ERROR OE_SO_ERROR
#define SO_KEEPALIVE OE_SO_KEEPALIVE
#define SO_RCVBUF OE_SO_RCVBUF
#define SO_SNDBUF OE_SO_SNDBUF
#define SOMAXCONN OE_SOMAXCONN
#define TCP_KEEPALIVE OE_TCP_KEEPALIVE
#define TCP_NODELAY OE_TCP_NODELAY
#endif

    oe_sockfd_t oe_socket(int domain, int type, int protocol);

    int oe_accept(
        _In_ oe_sockfd_t s,
        _Out_writes_bytes_(*addrlen) struct oe_sockaddr* addr,
        _Inout_ oe_socklen_t* addrlen);

    int oe_bind(
        _In_ oe_sockfd_t s,
        _In_reads_bytes_(namelen) const oe_sockaddr* name,
        oe_socklen_t namelen);

    oe_sockfd_t oe_connect(
        _In_ oe_sockfd_t s,
        _In_reads_bytes_(namelen) const oe_sockaddr* name,
        oe_socklen_t namelen);

    // void oe_freeaddrinfo(_In_ oe_addrinfo* ailist);

    uint32_t oe_htonl(_In_ uint32_t hostLong);

    uint16_t oe_htons(_In_ uint16_t hostShort);

    uint32_t oe_inet_addr(_In_z_ const char* cp);

    int oe_listen(_In_ int s, _In_ int backlog);

    uint32_t oe_ntohl(_In_ uint32_t netLong);

    uint16_t oe_ntohs(_In_ uint16_t netShort);

    ssize_t oe_recv(
        _In_ int s,
        _Out_writes_bytes_(len) void* buf,
        _In_ size_t len,
        _In_ int flags);

    ssize_t oe_send(
        _In_ int s,
        _In_reads_bytes_(len) const void* buf,
        _In_ size_t len,
        _In_ int flags);

    int oe_setsockopt(
        _In_ oe_sockfd_t s,
        _In_ int level,
        _In_ int optname,
        _In_reads_bytes_(optlen) const void* optval,
        _In_ oe_socklen_t optlen);

    int oe_shutdown(_In_ int s, _In_ int how);

#endif
