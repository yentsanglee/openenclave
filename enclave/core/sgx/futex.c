// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/errno.h>
#include <openenclave/corelibc/linux/futex.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include "td.h"

int oe_futex(
    int* uaddr,
    int futex_op,
    int val,
    const struct oe_timespec* timeout,
    int* uaddr2,
    int val3)
{
    int ret = -1;
    oe_futex_args_t* args = NULL;
    td_t* td;

    oe_printf("uaddr=%d val=%d\n", *uaddr, val);
    return -1;

    if (!(td = oe_get_td()))
        goto done;

    if (!(args = oe_host_calloc(1, sizeof(*args))))
        goto done;

    /* Input */
    {
        args->ret = -1;
        args->err = 0;

        if (!(args->tcs = (uint64_t)td_to_tcs(td)))
            goto done;

        args->uaddr = uaddr;
        args->futex_op = futex_op;
        args->val = val;

        if (timeout)
        {
            memcpy(&args->timeout_buf, timeout, sizeof(args->timeout_buf));
            args->timeout = &args->timeout_buf;
        }
        else
        {
            memset(&args->timeout_buf, 0, sizeof(args->timeout_buf));
            args->timeout = NULL;
        }

        args->uaddr2 = uaddr2;
        args->val3 = val3;
    }

    /* Call */
    if (oe_ocall(OE_OCALL_FUTEX, (uint64_t)args, NULL) != OE_OK)
    {
        goto done;
    }

    /* Output */
    oe_errno = args->err;
    ret = (int)args->ret;

done:

    if (args)
        oe_host_free(args);

    return ret;
}
