// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/hostfs.h>
#include <sys/stat.h>
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

                strncat(
                    args->u.readdir.buf.d_name,
                    entry->d_name,
                    sizeof(args->u.readdir.buf.d_name) - 1);

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
        case OE_HOSTFS_STAT:
        {
            struct stat buf;

            if ((args->u.stat.ret = stat(args->u.stat.pathname, &buf)) == 0)
            {
                args->u.stat.buf.st_dev = buf.st_dev;
                args->u.stat.buf.st_ino = buf.st_ino;
                args->u.stat.buf.st_mode = buf.st_mode;
                args->u.stat.buf.st_nlink = buf.st_nlink;
                args->u.stat.buf.st_uid = buf.st_uid;
                args->u.stat.buf.st_gid = buf.st_gid;
                args->u.stat.buf.st_rdev = buf.st_rdev;
                args->u.stat.buf.st_size = buf.st_size;
                args->u.stat.buf.st_blksize = buf.st_blksize;
                args->u.stat.buf.st_blocks = buf.st_blocks;
                args->u.stat.buf.st_atim.tv_sec = buf.st_atim.tv_sec;
                args->u.stat.buf.st_atim.tv_nsec = buf.st_atim.tv_nsec;
                args->u.stat.buf.st_mtim.tv_sec = buf.st_mtim.tv_sec;
                args->u.stat.buf.st_mtim.tv_nsec = buf.st_mtim.tv_nsec;
                args->u.stat.buf.st_ctim.tv_sec = buf.st_ctim.tv_sec;
                args->u.stat.buf.st_ctim.tv_nsec = buf.st_ctim.tv_nsec;
            }

            break;
        }
        case OE_HOSTFS_LINK:
        {
            args->u.link.ret = link(args->u.link.oldpath, args->u.link.newpath);
            break;
        }
        case OE_HOSTFS_UNLINK:
        {
            args->u.unlink.ret = unlink(args->u.unlink.path);
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
