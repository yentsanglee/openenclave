// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/corelibc/sys/utsname.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/host_socket.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "../client.h"
#include "../server.h"

static pthread_once_t once;

void _initialize(void)
{
    oe_allocate_devid(OE_DEVID_HOST_SOCKET);
    oe_set_devid_device(OE_DEVID_HOST_SOCKET, oe_socket_get_hostsock());
    oe_set_default_socket_devid(OE_DEVID_HOST_SOCKET);

    struct oe_utsname buf;
    OE_TEST(oe_uname(&buf) == 0);
    printf("sysname=%s\n", buf.sysname);
    printf("nodename=%s\n", buf.nodename);
    printf("release=%s\n", buf.release);
    printf("version=%s\n", buf.version);
    printf("machine=%s\n", buf.machine);
#ifdef _GNU_SOURCE
    printf("domainname=%s\n", buf.domainname);
#endif

    {
        char buf[1024];
        OE_TEST(gethostname(buf, sizeof(buf)) == 0);
        printf("HOSTNAME{%s}\n", buf);
        fflush(stdout);
    }

    {
        char buf[1024];
        OE_TEST(getdomainname(buf, sizeof(buf)) == 0);
        printf("DOMAINNAME{%s}\n", buf);
        fflush(stdout);
    }
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
