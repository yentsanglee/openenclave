// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/corelibc/sys/utsname.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "../client.h"
#include "../server.h"

static pthread_once_t once;

void _initialize(void)
{
    oe_set_default_socket_devid(OE_DEVID_HOST_SOCKET);
}

void run_enclave_server(uint16_t port)
{
    pthread_once(&once, _initialize);
    run_server(port);
}

void run_enclave_client(uint16_t port)
{
    pthread_once(&once, _initialize);
    run_client(port);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
