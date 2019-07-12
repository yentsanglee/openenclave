// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_SOCKET_H
#define _VE_ENCLAVE_SOCKET_H

#include "common.h"

#define VE_AF_LOCAL 1
#define VE_SOCK_STREAM 1

typedef uint32_t ve_socklen_t;

struct ve_iovec
{
    void* iov_base;
    size_t iov_len;
};

#if defined(__linux__)
struct ve_msghdr
{
    void* msg_name;
    ve_socklen_t msg_namelen;
    struct ve_iovec* msg_iov;
    int msg_iovlen, __pad1;
    void* msg_control;
    ve_socklen_t msg_controllen;
    ve_socklen_t __pad2;
    int msg_flags;
};
#endif

#if defined(__linux__)
struct ve_cmsghdr
{
    ve_socklen_t cmsg_len;
    int __pad1;
    int cmsg_level;
    int cmsg_type;
};
#endif

ssize_t ve_recvmsg(int sockfd, struct ve_msghdr* msg, int flags);

int ve_socketpair(int domain, int type, int protocol, int sv[2]);

#endif /* _VE_ENCLAVE_SOCKET_H */
