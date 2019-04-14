// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/hostfs.h>
#include <openenclave/internal/print.h>

typedef struct _file
{
    struct _oe_device base;
    uint64_t impl[8];
} file_t;

static file_t _in;
static file_t _out;
static file_t _err;

int oe_initialize_console_devices(void)
{
    int ret = -1;
    oe_device_t* hostfs;
    oe_device_t* in = NULL;
    oe_device_t* out = NULL;
    oe_device_t* err = NULL;

    /* If the local file_t struct is too small. */
    if (sizeof(file_t) < oe_get_hostfs_file_struct_size())
        goto done;

    /* Get the hostfs singleton instance. */
    if (!(hostfs = oe_fs_get_hostfs()))
        goto done;

    /* Open stdin. */
    if (!(in = hostfs->ops.fs->open(hostfs, "/dev/stdin", OE_O_RDONLY, 0)))
        goto done;

    /* Open stdout. */
    if (!(out = hostfs->ops.fs->open(hostfs, "/dev/stdout", OE_O_WRONLY, 0)))
        goto done;

    /* Open stderr. */
    if (!(err = hostfs->ops.fs->open(hostfs, "/dev/stderr", OE_O_WRONLY, 0)))
        goto done;

    /* Copy to static memory to avoid circular at-exit cleanup later. */
    memcpy(&_in, in, oe_get_hostfs_file_struct_size());
    memcpy(&_out, out, oe_get_hostfs_file_struct_size());
    memcpy(&_err, err, oe_get_hostfs_file_struct_size());

    /* Set the stdin device. */
    if (!oe_set_fd_device(OE_STDIN_FILENO, &_in.base))
        goto done;

    /* Set the stdout device. */
    if (!oe_set_fd_device(OE_STDOUT_FILENO, &_out.base))
        goto done;

    /* Set the stderr device. */
    if (!oe_set_fd_device(OE_STDERR_FILENO, &_err.base))
        goto done;

    /* Release the heap allocated structures. */
    oe_free(in);
    oe_free(out);
    oe_free(err);

    in = NULL;
    out = NULL;
    err = NULL;

    ret = 0;

done:

    if (in)
        in->ops.base->close(in);

    if (out)
        out->ops.base->close(out);

    if (err)
        err->ops.base->close(err);

    return ret;
}
