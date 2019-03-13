// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void run_server(uint16_t port, FILE* log)
{
    int listen_sd;
    int client_sd;
    char buf[1024];
    bool quit = false;

    /* Create the listener socket. */
    if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        assert("socket() failed" == NULL);
    }

    /* Reuse this server address. */
    {
        const int opt = 1;
        const socklen_t opt_len = sizeof(opt);

        if (setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, &opt, opt_len) != 0)
        {
            assert("setsockopt() failed" == NULL);
        }
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
        {
            int tmp = errno;
            printf("bind failed: %d\n", tmp);
            assert("bind() failed" == NULL);
        }

        if (listen(listen_sd, backlog) != 0)
        {
            assert("connect() failed" == NULL);
        }
    }

    while (!quit)
    {
        ssize_t n;

        if ((client_sd = accept(listen_sd, NULL, NULL)) < 0)
        {
            assert("accept() failed" == NULL);
        }

        for (;;)
        {
            if ((n = read(client_sd, buf, sizeof(buf))) < 0)
            {
                assert("read() failed" == NULL);
            }

            if (n > 0)
            {
                if (strncmp(buf, "quit", 4) == 0)
                {
                    quit = true;
                    break;
                }

                if (fwrite(buf, 1, n, log) != n)
                {
                    assert("fwrite() failed" == NULL);
                }

                fflush(log);

                if (write(client_sd, buf, (size_t)n) != n)
                {
                    assert("write() failed" == NULL);
                }
            }
        }
    }

    sleep(1);

    if (close(client_sd) != 0)
    {
        assert("close() failed" == NULL);
    }

    if (close(listen_sd) != 0)
    {
        assert("close() failed" == NULL);
    }
}

int main(int argc, const char* argv[])
{
    FILE* log;
    FILE* log2;

    /* Mount the insecure filesystem. */
    if (mount("/", "/", "hostfs", 0, NULL) != 0)
    {
        fprintf(stderr, "%s: mount() 1: failed\n", argv[0]);
        exit(1);
    }

    /* Create the /mnt/sgxfs directory. */
    mkdir("/mnt/sgxfs", 0777);

    /* Mount the secure filesystem. */
    if (mount("/mnt/sgxfs", "/mnt/sgxfs", "sgxfs", 0, NULL) != 0)
    {
        fprintf(stderr, "%s: mount() 2: failed\n", argv[0]);
        exit(1);
    }

    /* Open a log file. */
    if (!(log = fopen("/tmp/log", "w")))
    {
        fprintf(stderr, "%s: fopen() failed\n", argv[0]);
        exit(1);
    }

    /* Run the echo server. */
    run_server(33333, log);

    /* Write a secrte to the secure log file. */
    // int n = fprintf(log2, "secret\n");
    // assert(n == 7);

    fclose(log);
    // fclose(log2);

    exit(0);
}
