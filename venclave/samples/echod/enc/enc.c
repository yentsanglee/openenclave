// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <openenclave/enclave.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

OE_PRINTF_FORMAT(1, 2)
static void error_exit(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "enclave: error: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    abort();
}

static int recv_n(int fd, void* buf, size_t count)
{
    int ret = -1;
    uint8_t* p = (uint8_t*)buf;
    size_t n = count;

    if (fd < 0 || !buf)
        goto done;

    while (n > 0)
    {
        ssize_t r = recv(fd, p, n, 0);

        if (r <= 0)
            goto done;

        p += r;
        n -= (size_t)r;
    }

    ret = 0;

done:
    return ret;
}

static int send_n(int fd, const void* buf, size_t count)
{
    int ret = -1;
    const uint8_t* p = (const uint8_t*)buf;
    size_t n = count;

    if (fd < 0 || !buf)
        goto done;

    while (n > 0)
    {
        ssize_t r = send(fd, p, n, 0);

        if (r <= 0)
            goto done;

        p += r;
        n -= (size_t)r;
    }

    ret = 0;

done:
    return ret;
}

void echod_server(uint16_t port)
{
    int listener;

    /* Create the listener socket. */
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        error_exit("socket() failed: errno=%d", errno);

    /* Reuse this server address. */
    {
        const int opt = 1;
        const socklen_t opt_len = sizeof(opt);

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len) != 0)
            error_exit("setsockopt() failed: errno=%d", errno);
    }

    /* Listen on this address. */
    {
        struct sockaddr_in addr;
        const int backlog = 10;

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);

        if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) != 0)
            error_exit("bind() failed: errno=%d", errno);

        if (listen(listener, backlog) != 0)
            error_exit("listen() failed: errno=%d", errno);
    }

    /* Accept-recv-send-close until a zero value is received. */
    for (;;)
    {
        int client;
        uint64_t value;

        if ((client = accept(listener, NULL, NULL)) < 0)
            error_exit("accept() failed: errno=%d", errno);

        if (recv_n(client, &value, sizeof(value)) != 0)
            error_exit("recv_n() failed: errno=%d", errno);

        printf("value=%lu\n", value);

        if (send_n(client, &value, sizeof(value)) != 0)
            error_exit("send_n() failed: errno=%d", errno);

        close(client);

        if (value == 0)
            break;
    }

    close(listener);
}

void load_module()
{
    oe_load_module_host_socket_interface();
}

void echod_server_ecall(uint16_t port)
{
    load_module();
    echod_server(port);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
