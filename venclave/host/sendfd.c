// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <openenclave/bits/defs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int send_fd(int sock, int fd)
{
    int ret = -1;
    struct iovec iov[1];
    struct msghdr msg;
    uint32_t magic = 0x5E2DFD00;
    OE_ALIGNED(sizeof(uint64_t)) char cmsg_buf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr* cmsg = (struct cmsghdr*)cmsg_buf;
    const size_t cmsg_len = CMSG_LEN(sizeof(int));

    if (sock < 0 || fd < 0)
        goto done;

    iov[0].iov_base = &magic;
    iov[0].iov_len = sizeof(magic);

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg_len;

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = cmsg_len;
    *(int*)CMSG_DATA(cmsg) = fd;

    if (sendmsg(sock, &msg, 0) != sizeof(magic))
        goto done;

    ret = 0;

done:
    return ret;
}

int recv_fd(int sock)
{
    int ret = -1;
    struct iovec iov[1];
    struct msghdr msg;
    const uint64_t MAGIC = 0x5E2DFD00;
    uint64_t magic = 0;
    OE_ALIGNED(sizeof(uint64_t)) char cmsg_buf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr* cmsg = (struct cmsghdr*)cmsg_buf;
    const size_t cmsg_len = CMSG_LEN(sizeof(int));

    if (sock < 0)
        goto done;

    iov[0].iov_base = &magic;
    iov[0].iov_len = sizeof(magic);

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg_len;

    if (recvmsg(sock, &msg, 0) != sizeof(magic))
        goto done;

    if (memcmp(&magic, &MAGIC, sizeof(magic)) != 0)
        goto done;

    memcpy(&ret, CMSG_DATA(cmsg), sizeof(ret));

done:
    return ret;
}
