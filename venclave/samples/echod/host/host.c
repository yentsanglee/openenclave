// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <netinet/in.h>
#include <openenclave/host.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "echod_u.h"

static const char* arg0;

const uint16_t PORT = 12345;

OE_PRINTF_FORMAT(1, 2)
static void error_exit(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
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

static void run_client(uint16_t port, uint64_t value)
{
    int sock;

    /* Create the client socket. */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        error_exit("socket() failed: errno=%d", errno);

    /* Connect to the server. */
    {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0)
            error_exit("connectd() failed: errno=%d", errno);
    }

    /* write/read "hello" to/from  the server. */
    {
        uint64_t tmp;

        if (send_n(sock, &value, sizeof(value)) != 0)
            error_exit("send_n() failed: errno=%d", errno);

        if (recv_n(sock, &tmp, sizeof(tmp)) != 0)
            error_exit("recv_n() failed: errno=%d", errno);

        if (tmp != value)
            error_exit("comparision failed");
    }

    close(sock);
}

static void* server_thread(void* arg)
{
    oe_enclave_t* enclave = (oe_enclave_t*)arg;
    oe_result_t result;

    if ((result = echod_server_ecall(enclave, PORT)) != OE_OK)
        error_exit("echod_server_ecall() failed: result=%u", result);

    return NULL;
}

static void _close_standard_devices(void)
{
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    oe_result_t result;
    const uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    const oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    oe_enclave_t* enclave;
    int rval;
    pthread_t thread;

    /* Check the command line arguments. */
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH\n", argv[0]);
        return 1;
    }

    /* Create the enclave. */
    if ((result = oe_create_echod_enclave(
             argv[1], type, flags, NULL, 0, &enclave)) != OE_OK)
    {
        error_exit("oe_create_echod_enclave() failed: result=%u", result);
    }

    /* Run the server in its own thread. */
    if ((rval = pthread_create(&thread, NULL, server_thread, enclave)) != 0)
        error_exit("pthread_create() failed: rval=%d", rval);

    /* Give server time to start. */
    sleep(1);

    /* Run the client. */
    for (size_t value = 1; value <= 10; value++)
        run_client(PORT, value);

    /* Terminate the server. */
    run_client(PORT, 0);

    /* Join with the server thread. */
    pthread_join(thread, NULL);

    /* Terminate the enclave. */
    if ((result = oe_terminate_enclave(enclave)) != OE_OK)
        error_exit("oe_terminate_enclave() failed: result=%u", result);

    _close_standard_devices();

    return 0;
}
