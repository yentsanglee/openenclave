// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include "../common/hostblkdev.h"

static void _handle_open(oefs_oefs_ocall_args_t* args)
{
    if (args && args->open.path)
    {
        FILE* stream;

        args->open.handle = NULL;

        if (!(stream = fopen(args->open.path, "w+")))
            return;

        args->open.handle = stream;
    }
}

static void _handle_close(oefs_oefs_ocall_args_t* args)
{
    if (args && args->close.handle)
    {
        fclose((FILE*)args->close.handle);
    }
}

static void _handle_get(oefs_oefs_ocall_args_t* args)
{
    if (args)
    {
        FILE* stream;

        args->get.ret = -1;

        if (!(stream = (FILE*)args->get.handle))
            return;

        /* Read the given block. */
        {
            long offset = args->get.blkno * OEFS_BLOCK_SIZE;
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

static void _handle_put(oefs_oefs_ocall_args_t* args)
{
    if (args)
    {
        FILE* stream;

        args->put.ret = -1;

        if (!(stream = (FILE*)args->put.handle))
            return;

        /* Write the given block. */
        {
            const long offset = args->put.blkno * OEFS_BLOCK_SIZE;

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

static void _handle_oefs_ocall(void* args_)
{
    oefs_oefs_ocall_args_t* args = (oefs_oefs_ocall_args_t*)args_;

    switch (args->op)
    {
        case OEFS_HOSTBLKDEV_OPEN:
        {
            _handle_open(args);
            break;
        }
        case OEFS_HOSTBLKDEV_CLOSE:
        {
            _handle_close(args);
            break;
        }
        case OEFS_HOSTBLKDEV_GET:
        {
            _handle_get(args);
            break;
        }
        case OEFS_HOSTBLKDEV_PUT:
        {
            _handle_put(args);
            break;
        }
    }
}

extern void (*oe_handle_oefs_ocall_callback)(void* args);

void oe_install_oefs(void)
{
    oe_handle_oefs_ocall_callback = _handle_oefs_ocall;
}
