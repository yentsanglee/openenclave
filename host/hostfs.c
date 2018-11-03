// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
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
            args->u.open.ret = open(
                args->u.open.pathname, args->u.open.flags, args->u.open.mode);
            break;
        }
        case OE_HOSTFS_LSEEK:
        {
            args->u.lseek.ret = lseek(
                args->u.lseek.fd, args->u.lseek.offset, args->u.lseek.whence);
            break;
        }
        case OE_HOSTFS_READ:
        {
            args->u.read.ret =
                read(args->u.read.fd, args->u.read.buf, args->u.read.count);
            break;
        }
        case OE_HOSTFS_WRITE:
        {
            args->u.write.ret =
                write(args->u.write.fd, args->u.write.buf, args->u.write.count);
            break;
        }
        case OE_HOSTFS_CLOSE:
        {
            args->u.close.ret = close(args->u.close.fd);
            break;
        }
        case OE_HOSTFS_OPENDIR:
        {
            args->u.opendir.dir = opendir(args->u.opendir.name);
            break;
        }
        case OE_HOSTFS_READDIR:
        {
            struct dirent* entry = readdir((DIR*)args->u.readdir.dir);

            if (entry)
            {
                args->u.readdir.buf.d_ino = entry->d_ino;
                args->u.readdir.buf.d_off = entry->d_off;
                args->u.readdir.buf.d_reclen = entry->d_reclen;
                args->u.readdir.buf.d_type = entry->d_type;

                *args->u.readdir.buf.d_name = '\0';

                strncat(args->u.readdir.buf.d_name, entry->d_name,
                    sizeof(args->u.readdir.buf.d_name)-1);

                args->u.readdir.entry = &args->u.readdir.buf;
            }
            else
            {
                args->u.readdir.entry = NULL;
            }

            break;
        }
        case OE_HOSTFS_CLOSEDIR:
        {
            args->u.closedir.ret = closedir((DIR*)args->u.closedir.dir);
            break;
        }
        default:
        {
            errno = EINVAL;
            break;
        }
    }

    args->err = errno;
}
