// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <stdio.h>

#if defined(__linux__)
#include <errno.h>
#include <linux/futex.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif

#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include "../ocalls.h"
#include "ocalls.h"

#if !defined(__linux__)
#error "unsupported"
#endif

void oe_handle_futex(oe_enclave_t* enclave, uint64_t arg)
{
    oe_futex_args_t* args = (oe_futex_args_t*)arg;

    if (args)
    {
        printf("futex.host: %d ? %d\n", *args->uaddr, args->val);
        fflush(stdout);

        EnclaveEvent* event = GetEnclaveEvent(enclave, args->tcs);
        assert(event);
        assert(!args->timeout);

        errno = 0;
        args->ret = syscall(
            __NR_futex,
            args->uaddr,
            args->futex_op,
            args->val,
            args->timeout,
            args->uaddr2,
            args->val3);
        args->err = errno;
    }
}
