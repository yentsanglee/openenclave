// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <fcntl.h>
#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/hostfs.h>
#include <unistd.h>
#include "ocalls.h"

void oe_handle_hostfs(oe_enclave_t* enclave, uint64_t arg)
{
    oe_hostfs_args_t* args = (oe_hostfs_args_t*)arg;

    if (!args)
        return;

    errno = 0;

    switch (args->op)
    {
        case OE_HOSTFS_OPEN:
        {
            args->ret = open(
                args->u.open.pathname, args->u.open.flags, args->u.open.mode);
            break;
        }
        case OE_HOSTFS_LSEEK:
        {
            args->ret = lseek(
                args->u.lseek.fd, args->u.lseek.offset, args->u.lseek.whence);
            break;
        }
        case OE_HOSTFS_READ:
        {
            args->ret =
                read(args->u.read.fd, args->u.read.buf, args->u.read.count);
            break;
        }
        case OE_HOSTFS_WRITE:
        {
            args->ret =
                write(args->u.write.fd, args->u.write.buf, args->u.write.count);
            break;
        }
        case OE_HOSTFS_CLOSE:
        {
            args->ret = close(args->u.close.fd);
            break;
        }
        default:
        {
            errno = EINVAL;
            args->ret = -1;
            break;
        }
    }

    args->err = errno;
}
