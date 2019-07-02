// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/entropy.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/time.h>
#include <stdio.h>
#include "vsample1_t.h"

uint64_t test_ecall(uint64_t x)
{
    uint64_t retval;
    static oe_mutex_t _mutex = OE_MUTEX_INITIALIZER;
    static oe_spinlock_t _spin = OE_SPINLOCK_INITIALIZER;

    oe_spin_lock(&_spin);
    oe_sleep_msec(10);
    oe_spin_unlock(&_spin);

    printf("test_ecall{%s}\n", "some string");

    {
        uint8_t buf[64];
        oe_mutex_lock(&_mutex);
        oe_get_entropy(buf, sizeof(buf));
        oe_hex_dump(buf, sizeof(buf));
        oe_mutex_unlock(&_mutex);
    }

    if (test_ocall(&retval, x) != OE_OK)
    {
        oe_host_printf("test_ocall() failed\n");
        oe_abort();
    }

    return retval;
}

int test1(char* in, char out[100])
{
    if (in)
        oe_strlcpy(out, in, 100);

    return 0;
}

int test_time(void)
{
    extern time_t oe_time(time_t * tloc);

    for (size_t i = 0; i < 10; i++)
    {
        oe_host_printf("sleep...\n");
        oe_sleep_msec(1000);

        oe_host_printf("time.secs=%lu\n", oe_time(NULL));
        oe_host_printf("time.msec=%lu\n", oe_get_time());
    }

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    256,  /* StackPageCount */
    8);   /* TCSCount */
