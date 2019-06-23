// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <unistd.h>
#include "enclave.h"
#include "err.h"
#include "heap.h"

const char* __ve_arg0;

int main(int argc, const char* argv[])
{
    __ve_arg0 = argv[0];
    ve_enclave_t* enclave = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        return 1;
    }

    /* Create the host heap to be shared with enclaves. */
    if (ve_heap_create(VE_HEAP_SIZE) != 0)
        err("failed to allocate shared memory");

    if (ve_create_enclave(argv[1], &enclave) != 0)
        err("failed to create enclave");

    if (ve_terminate_enclave(enclave) != 0)
        err("failed to create enclave");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}
