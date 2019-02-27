// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/corelibc/sys/syscall.h>
#include <sys/socket.h>
#include <sys/syscall.h>

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    return (int)oe_syscall(SYS_accept, (long)sockfd, (long)addr, (long)addrlen);
}
