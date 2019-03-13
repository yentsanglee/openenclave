// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const char* argv0;

void err(const char* format, ...)
{
    fprintf(stderr, "%s: ", argv0);

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fprintf(stderr, "\n");
    exit(1);
}

void run_server(FILE* log, FILE* log2)
{
    int listen_sd;
    int client_sd;
    char buf[1024];
    bool quit = false;
    const uint16_t MIN_PORT = 30000;
    const uint16_t MAX_PORT = MIN_PORT + 10;
    uint16_t port;
    const int backlog = 10;

    /* Create the listener socket. */
    if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        err("socket() failed");

    /* Reuse this server address. */
    {
        const int opt = 1;
        const socklen_t opt_len = sizeof(opt);

        if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len) != 0)
            err("setsockopt() failed");
    }

    /* Listen on the first available port. */
    for (port = MIN_PORT; port <= MAX_PORT; port++)
    {
        struct sockaddr_in addr;

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);

        if (bind(listen_sd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
            break;
    }

    if (port > MAX_PORT)
        err("bind() failed");

    if (listen(listen_sd, backlog) != 0)
        err("listen() failed");

    printf("Listening on port %u\n", port);

    while (!quit)
    {
        ssize_t n;

        if ((client_sd = accept(listen_sd, NULL, NULL)) < 0)
            err("accept() failed");

        for (;;)
        {
            if ((n = read(client_sd, buf, sizeof(buf))) < 0)
                err("read() failed");

            if (n > 0)
            {
                if (strncmp(buf, "quit", 4) == 0)
                {
                    quit = true;
                    break;
                }

                if (fwrite(buf, 1, n, log) != n)
                    err("fwrite(log) failed");

                if (fwrite(buf, 1, n, log2) != n)
                    err("fwrite(log2) failed");

                fflush(log);
                fflush(log2);

                if (write(client_sd, buf, (size_t)n) != n)
                    err("write() failed");
            }
        }
    }

    sleep(3);

    if (close(client_sd) != 0)
        err("close(client_sd) failed");

    if (close(listen_sd) != 0)
        err("close(listen_sd) failed");
}

int main(int argc, const char* argv[])
{
    argv0 = argv[0];
    FILE* log;
    FILE* log2;
    size_t n;

    /* Print the command line arguments. */
    for (int i = 0; i < argc; i++)
        printf("argv[%d]=%s\n", i, argv[i]);

    /* Mount the insecure filesystem. */
    if (mount("/", "/", "hostfs", 0, NULL) != 0)
        err("mount(/) failed");

    /* Create the /mnt/sgxfs directory. */
    mkdir("/mnt/sgxfs", 0777);

    /* Mount the secure filesystem. */
    if (mount("/mnt/sgxfs", "/mnt/sgxfs", "sgxfs", 0, NULL) != 0)
        err("mount(/mnt/sgxfs) failed");

    /* Open a non-secure log file. */
    if (!(log = fopen("/tmp/log", "w")))
        err("fopen(/tmp/log) failed\n");

    /* Open a secure log file. */
    if (!(log2 = fopen("/mnt/sgxfs/log", "w")))
        err("fopen(/mnt/sgxfs/log) failed\n");

    /* Run the echo server. */
    run_server(log, log2);

    fclose(log);
    fclose(log2);

    exit(0);
}
