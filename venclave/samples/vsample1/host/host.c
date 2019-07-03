// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <pthread.h>
#include <unistd.h>
#include "vsample1_u.h"

const char* arg0;

OE_PRINTF_FORMAT(1, 2)
void err(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

uint64_t test_ocall(uint64_t x)
{
#if 0
    printf("test_ocall(): thread=%lu x=%lu\n", pthread_self(), x);
    fflush(stdout);
#endif
    return x;
}

static void _close_standard_devices(void)
{
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void* _thread_func(void* arg)
{
    oe_enclave_t* enclave = (oe_enclave_t*)arg;
    const uint64_t N = 10;

    printf("=== new thread\n");
    fflush(stdout);

    /* Call test_ecall() N times. */
    for (uint64_t i = 0; i < N; i++)
    {
        oe_result_t result;
        uint64_t retval = 0;

        if ((result = test_ecall(enclave, &retval, i)) != OE_OK)
            err("test1() failed: %s\n", oe_result_str(result));

        if (retval != i)
            err("test1() failed: expected retval=%lu", retval);
    }

    return NULL;
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    oe_result_t result;
    oe_enclave_t* enclave = NULL;
    const size_t N = 8;
    pthread_t threads[N];

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s program path\n", argv[0]);
        return 1;
    }

    if ((result = oe_create_vsample1_enclave(
             argv[1],
             OE_ENCLAVE_TYPE_SGX,
             OE_ENCLAVE_FLAG_DEBUG,
             NULL,
             0,
             &enclave)) != OE_OK)
    {
        err("oe_create_enclave() failed: %s\n", oe_result_str(result));
    }

    for (size_t i = 0; i < N; i++)
    {
        if (pthread_create(&threads[i], NULL, _thread_func, enclave) != 0)
            err("pthread_create() failed");
    }

    for (size_t i = 0; i < N; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
            err("pthread_join() failed");
    }

    /* Test file systems support. */
    {
        if ((result = test_files(enclave, argv[2])) != OE_OK)
            err("test_files() failed: %s\n", oe_result_str(result));
    }

#if 0
    {
        int retval;

        if ((result = test_time(enclave, &retval)) != OE_OK || retval != 0)
            err("test_time() failed: %s: %d\n", oe_result_str(result), retval);
    }
#endif

    if ((result = oe_terminate_enclave(enclave)) != OE_OK)
        err("oe_terminate_enclave() failed: %s\n", oe_result_str(result));

    _close_standard_devices();

    return 0;
}
