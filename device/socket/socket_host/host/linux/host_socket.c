// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../common/hostsockargs.h"

void (*oe_handle_hostsock_ocall_callback)(void*);

static void _handle_hostsock_ocall(void* args_)
{
    oe_hostsock_args_t* args = (oe_hostsock_args_t*)args_;

    /* ATTN: handle errno propagation. */

    if (!args)
        return;

    args->err = 0;
    switch (args->op)
    {
        case OE_HOSTSOCK_OP_NONE:
        {
            break;
        }
        case OE_HOSTSOCK_OP_SOCKET:
        {
            args->u.socket.ret = socket(
                args->u.socket.domain,
                args->u.socket.type,
                args->u.socket.protocol);
            break;
        }
        case OE_HOSTSOCK_OP_CLOSE:
        {
            args->u.close.ret = close(args->u.close.fd);
            break;
        }
        case OE_HOSTSOCK_OP_READ:
        {
            args->u.read.ret =
                read(args->u.read.fd, args->buf, args->u.read.count);
            break;
        }
        case OE_HOSTSOCK_OP_WRITE:
        {
            args->u.read.ret =
                write(args->u.read.fd, args->buf, args->u.read.count);
            break;
        }
        case OE_HOSTSOCK_OP_RECV:
        {
            args->u.recv.ret = recv(
                args->u.recv.fd,
                args->buf,
                args->u.recv.count,
                args->u.recv.flags);
            break;
        }
        case OE_HOSTSOCK_OP_SEND:
        {
            args->u.send.ret = send(
                args->u.send.fd,
                args->buf,
                args->u.send.count,
                args->u.send.flags);
            break;
        }
    }
    args->err = errno;
}

void oe_fs_install_hostsock(void)
{
    oe_handle_hostsock_ocall_callback = _handle_hostsock_ocall;
}
