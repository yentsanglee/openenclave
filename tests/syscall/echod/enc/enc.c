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

void echod_server(uint16_t port)
{
    int listen_sd;
    int client_sd;
    char buf[1024];
    bool quit = false;

    /* Create the listener socket. */
    if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        error_exit("socket() failed: errno=%d", errno);

    /* Reuse this server address. */
    {
        const int opt = 1;
        const socklen_t opt_len = sizeof(opt);

        if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len) != 0)
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

        if (bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
            error_exit("bind() failed: errno=%d", errno);

        if (listen(listen_sd, backlog) != 0)
            error_exit("listen() failed: errno=%d", errno);
    }

    while (!quit)
    {
        ssize_t n;

        if ((client_sd = accept(listen_sd, NULL, NULL)) < 0)
            error_exit("accept() failed: errno=%d", errno);

        for (;;)
        {
            if ((n = recv(client_sd, buf, sizeof(buf), 0)) < 0)
                error_exit("recv() failed: errno=%d", errno);

            if (n > 0)
            {
                if (strncmp(buf, "quit", 4) == 0)
                {
                    quit = true;
                    break;
                }

                if (send(client_sd, buf, (size_t)n, 0) != n)
                    error_exit("send() failed: errno=%d", errno);
            }
        }
    }

    sleep(1);

    if (close(client_sd) != 0)
        error_exit("close() failed: errno=%d", errno);

    if (close(listen_sd) != 0)
        error_exit("close() failed: errno=%d", errno);
}

void echod_server_ecall(uint16_t port)
{
    oe_load_module_host_socket_interface();
    echod_server(port);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
