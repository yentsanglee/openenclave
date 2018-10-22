#include "blockdevice.h"
#include <openenclave/internal/calls.h>
#include <stdio.h>

void oe_handle_open_block_device(
    oe_enclave_t* enclave, 
    uint64_t arg_in,
    uint64_t* arg_out)
{
    const char* device_name = (const char*)arg_in;
    FILE* file;

    if (arg_out)
        *arg_out = 0;

    if (!device_name || !arg_out)
        return;

    if (!(file = fopen(device_name, "rb")))
        return;

    *arg_out = (uint64_t)file;
}

void oe_handle_close_block_device(oe_enclave_t* enclave, uint64_t arg_in)
{
    FILE* file = (FILE*)arg_in;

    if (file)
        fclose(file);
}

void oe_handle_get_block_device(oe_enclave_t* enclave, uint64_t arg_in)
{
    typedef oe_ocall_get_block_device_args_t args_t;
    args_t* args = (args_t*)arg_in;
    FILE* stream;

    if (!args)
        return;

    args->ret = -1;

    if (!(stream = (FILE*)args->host_context))
        return;

    /* Read the given block. */
    {
        const long offset = args->blkno * OE_BLOCK_DEVICE_BLOCK_SIZE;

        if (fseek(stream, offset, SEEK_SET) != 0)
            return;

        const size_t count = sizeof(args->block);

        if (fread(args->block, 1, count, stream) != count)
            return;
    }

    args->ret = 0;
}

void oe_handle_put_block_device(oe_enclave_t* enclave, uint64_t arg_in)
{
    typedef oe_ocall_get_block_device_args_t args_t;
    args_t* args = (args_t*)arg_in;
    FILE* stream;

    if (!args)
        return;

    args->ret = -1;

    if (!(stream = (FILE*)args->host_context))
        return;

    /* Write the given block. */
    {
        const long offset = args->blkno * OE_BLOCK_DEVICE_BLOCK_SIZE;

        if (fseek(stream, offset, SEEK_SET) != 0)
            return;

        const size_t count = sizeof(args->block);

        if (fwrite(args->block, 1, count, stream) != count)
            return;
    }

    args->ret = 0;
}
