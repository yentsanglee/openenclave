// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <fcntl.h>
#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/hostsyscall.h>
#include <unistd.h>
#include "ocalls.h"

void oe_handle_host_syscall(oe_enclave_t* enclave, uint64_t arg)
{
    oe_host_syscall_args_t* args = (oe_host_syscall_args_t*)arg;

    if (!args)
        return;

    errno = 0;

    switch (args->num)
    {
        case OE_SYSCALL_open:
        {
            args->ret = open(
                args->u.open.pathname, args->u.open.flags, args->u.open.mode);
            break;
        }
        case OE_SYSCALL_close:
        {
            args->ret = close(args->u.close.fd);
            break;
        }
        case OE_SYSCALL_write:
        {
            args->ret =
                write(args->u.write.fd, args->u.write.buf, args->u.write.count);
            break;
        }
        case OE_SYSCALL_read:
        {
            args->ret =
                read(args->u.read.fd, args->u.read.buf, args->u.read.count);
            break;
        }
        case OE_SYSCALL_lseek:
        {
            args->ret =
                lseek(args->u.lseek.fd, args->u.lseek.offset, args->u.lseek.whence);
            break;
        }
        default:
        {
            args->ret = -1;
            break;
        }
    }

    args->err = errno;
}
