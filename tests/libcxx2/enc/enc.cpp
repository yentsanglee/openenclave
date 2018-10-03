// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <cstdlib>
#include <cstdio>
#include "../args.h"
#include "tests.h"

extern "C" void _exit(int status)
{
    oe_call_host("ocall_exit", (void*)(long)status);
    abort();
}

extern "C" void _Exit(int status)
{
    _exit(status);
    abort();
}

extern "C" void exit(int status)
{
    _exit(status);
    abort();
}

typedef void (*Handler)(int signal);

Handler signal(int signal, Handler)
{
    /* Ignore! */
    return NULL;
}

extern "C" int close(int fd)
{
    OE_TEST("close() panic" == NULL);
    return 0;
}

OE_ECALL void test(void* args_)
{
    args_t* args = (args_t*)args_;
    int ret = 0;

    args->ret = 0;

    printf("running: %s\n", "test1");

#include "tests.cpp"

    if (ret != 0)
        args->ret++;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    4096, /* HeapPageCount */
    4096, /* StackPageCount */
    2);   /* TCSCount */

