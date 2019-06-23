// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <fcntl.h>
#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "call.h"
#include "enclave.h"
#include "heap.h"
#include "hostmalloc.h"
#include "io.h"
#include "trace.h"

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

OE_PRINTF_FORMAT(1, 2)
void puterr(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    ve_enclave_t* enclave = NULL;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        exit(1);
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
