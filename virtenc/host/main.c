// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <unistd.h>
#include "enclave.h"
#include "err.h"
#include "heap.h"

#define VE_HEAP_SIZE (1024 * 1024)

const char* __ve_arg0;
int __ve_pid;
extern ve_heap_t __ve_heap;

int main1(int argc, const char* argv[])
{
    __ve_arg0 = argv[0];
    ve_enclave_t* enclave = NULL;
    ve_enclave_settings_t settings;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        return 1;
    }

    if ((__ve_pid = getpid()) < 0)
        err("getpid() failed");

    /* Create the host heap to be shared with enclaves. */
    if (ve_heap_create(&__ve_heap, VE_HEAP_SIZE) != 0)
        err("failed to allocate shared memory");

    if (ve_enclave_create(argv[1], &__ve_heap, &enclave) != 0)
        err("failed to create enclave");

    /* Get the enclave settings. */
    {
        /* Get the enclave settings. */
        if (ve_enclave_get_settings(enclave, &settings) != 0)
            err("failed to get settings");

        printf("host: num_heap_pages=%lu\n", settings.num_heap_pages);
        printf("host: num_stack_pages=%lu\n", settings.num_stack_pages);
        printf("host: num_tcs=%lu\n", settings.num_tcs);
    }

    /* Ping the main thread. */
    if (ve_enclave_ping(enclave, (uint64_t)-1, 0xF00DF00D) != 0)
        err("failed to ping enclave");

    for (uint64_t tcs = 0; tcs < settings.num_tcs; tcs++)
    {
        if (ve_enclave_ping(enclave, tcs, 0xF00DF00D) != 0)
            err("failed to ping enclave");
    }

    /* Run the built-in xor test. */
    if (ve_enclave_run_xor_test(enclave) != 0)
        err("ve_enclave_run_xor_test() failed");

    if (ve_enclave_terminate(enclave) != 0)
        err("failed to terminate enclave");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

int main2(int argc, const char* argv[])
{
    __ve_arg0 = argv[0];
    ve_enclave_t* enclave = NULL;
    ve_enclave_t* enclave2 = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        return 1;
    }

    if ((__ve_pid = getpid()) < 0)
        err("getpid() failed");

    /* Create the host heap to be shared with enclaves. */
    if (ve_heap_create(&__ve_heap, VE_HEAP_SIZE) != 0)
        err("failed to allocate shared memory");

    if (ve_enclave_create(argv[1], &__ve_heap, &enclave) != 0)
        err("failed to create enclave");

    if (ve_enclave_create(argv[1], &__ve_heap, &enclave2) != 0)
        err("failed to create enclave");

    if (ve_enclave_terminate(enclave) != 0)
        err("failed to terminate enclave");

    if (ve_enclave_terminate(enclave2) != 0)
        err("failed to terminate enclave");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

int main(int argc, const char* argv[])
{
    return main1(argc, argv);
}
