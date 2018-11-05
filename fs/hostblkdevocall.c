// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include "hostblkdev.h"

static void _handle_open(fs_hostblkdev_ocall_args_t* args)
{
    if (args && args->open.path)
    {
        FILE* stream;

        args->open.handle = NULL;

        if (!(stream = fopen(args->open.path, "r+")))
            return;

        args->open.handle = stream;
    }
}

static void _handle_close(fs_hostblkdev_ocall_args_t* args)
{
    if (args && args->close.handle)
    {
        fclose((FILE*)args->close.handle);
    }
}

static void _handle_get(fs_hostblkdev_ocall_args_t* args)
{
    if (args)
    {
        FILE* stream;

        args->get.ret = -1;

        if (!(stream = (FILE*)args->get.handle))
            return;

        /* Read the given block. */
        {
            long offset = args->get.blkno * FS_BLOCK_SIZE;
            size_t count;

            if (fseek(stream, offset, SEEK_SET) != 0)
                return;

            count = sizeof(args->get.blk);

            if (fread(args->get.blk.data, 1, count, stream) != count)
                return;
        }

        args->get.ret = 0;
    }
}

static void _handle_put(fs_hostblkdev_ocall_args_t* args)
{
    if (args)
    {
        FILE* stream;

        args->put.ret = -1;

        if (!(stream = (FILE*)args->put.handle))
            return;

        /* Write the given block. */
        {
            const long offset = args->put.blkno * FS_BLOCK_SIZE;

            if (fseek(stream, offset, SEEK_SET) != 0)
                return;

            const size_t count = sizeof(args->put.blk);

            if (fwrite(args->put.blk.data, 1, count, stream) != count)
                return;

            fflush(stream);
        }

        args->put.ret = 0;
    }
}

void fs_handle_hostblkdev_ocall(fs_hostblkdev_ocall_args_t* args)
{
    switch (args->op)
    {
        case FS_HOSTBLKDEV_OPEN:
        {
            _handle_open(args);
            break;
        }
        case FS_HOSTBLKDEV_CLOSE:
        {
            _handle_close(args);
            break;
        }
        case FS_HOSTBLKDEV_GET:
        {
            _handle_get(args);
            break;
        }
        case FS_HOSTBLKDEV_PUT:
        {
            _handle_put(args);
            break;
        }
    }
}
