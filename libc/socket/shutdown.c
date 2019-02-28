// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/corelibc/sys/syscall.h>
#include <sys/socket.h>
#include <sys/syscall.h>

int shutdown(int sockfd, int how)
{
    return (int)oe_syscall(SYS_shutdown, (long)sockfd, (long)how);
}
