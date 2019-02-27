/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#include <openenclave/enclave.h>
#include <openenclave/internal/time.h>

// enclave.h must come before socket.h
#include <openenclave/corelibc/arpa/inet.h>
#include <openenclave/corelibc/netinet/in.h>
#include <openenclave/corelibc/sys/socket.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/epoll.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/host_socket.h>

#include <epoll_test_t.h>
#include <stdio.h>
#include <string.h>

int ecall_device_init()
{
    (void)oe_allocate_devid(OE_DEVID_HOST_SOCKET);
    (void)oe_set_devid_device(OE_DEVID_HOST_SOCKET, oe_socket_get_hostsock());
    (void)oe_allocate_devid(OE_DEVID_EPOLL);
    (void)oe_set_devid_device(OE_DEVID_EPOLL, oe_epoll_get_epoll());
    return 0;
}

const char* print_socket_success(void)
{
    static const char* msg = "socket success";
    printf("%s\n", msg);
    return msg;
}

const char* print_file_success(void)
{
    static const char* msg = "file success";
    printf("%s\n", msg);
    return msg;
}

/* This client connects to an echo server, sends a text message,
 * and outputs the text reply.
 */
int ecall_run_client(void)
{
    int sockfd = 0;
    int file_fd = 0;
    // ssize_t n = 0;
    char recv_buff[1024];
    size_t buff_len = sizeof(recv_buff);
    struct oe_sockaddr_in serv_addr = {0};
    static const int MAX_EVENTS = 20;
    struct oe_epoll_event event = {0};
    struct oe_epoll_event events[MAX_EVENTS] = {{0}};
    int epoll_fd = oe_epoll_create1(0);

    if (epoll_fd == -1)
    {
        printf("Failed to create epoll file descriptor\n");
        return OE_FAILURE;
    }

    if (epoll_fd == -1)
    {
        printf("Failed to create epoll file descriptor\n");
        return OE_FAILURE;
    }

    memset(recv_buff, '0', buff_len);
    printf("create socket\n");
    if ((sockfd = oe_socket(OE_AF_INET, OE_SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return OE_FAILURE;
    }
    serv_addr.sin_family = OE_AF_INET;
    serv_addr.sin_addr.s_addr = oe_htonl(OE_INADDR_LOOPBACK);
    serv_addr.sin_port = oe_htons(1642);

    printf("socket fd = %d\n", sockfd);
    printf("Connecting...\n");
    int retries = 0;
    static const int max_retries = 4;
    while (oe_connect(
               sockfd, (struct oe_sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        if (retries++ > max_retries)
        {
            printf("\n Error : Connect Failed \n");
            oe_close(sockfd);
            return OE_FAILURE;
        }
        else
        {
            printf("Connect Failed. Retrying \n");
        }
    }

    const int flags = OE_O_NONBLOCK | OE_O_RDONLY;
    file_fd = oe_open("/tmp/test", flags, 0);

    printf("polling...\n");
    if (file_fd >= 0)
    {
        event.events = OE_EPOLLIN;
        event.data.ptr = (void*)print_file_success;

        if (oe_epoll_ctl(epoll_fd, OE_EPOLL_CTL_ADD, 0, &event))
        {
            fprintf(stderr, "Failed to add file descriptor to epoll\n");
            oe_close(epoll_fd);
            return 1;
        }
    }

    event.events = 0x3c7;
    event.data.ptr = (void*)print_socket_success;
    if (oe_epoll_ctl(epoll_fd, OE_EPOLL_CTL_ADD, sockfd, &event))
    {
        fprintf(stderr, "Failed to add file descriptor to epoll\n");
        oe_close(epoll_fd);
        return 1;
    }

    int nfds = 0;
    do
    {
        /*while*/ if ((nfds = oe_epoll_wait(epoll_fd, events, 20, 30000)) < 0)
        {
            printf("error.\n");
        }
        else
        {
            printf("input from %d fds\n", nfds);

            for (int i = 0; i < nfds; i++)
            {
                const char* (*func)(void) =
                    (const char* (*)(void))events[i].data.ptr;
                (void)(*func)();

#if 0
                n = oe_read(events[i].data.fd, recv_buff, buff_len);
                recv_buff[n] = 0;
                printf(
                    "received data %s from fd %d\n", recv_buff, events[i].data.fd);
#endif
            }
        }

    } while (nfds >= 0);

    // oe_close(sockfd);
    // oe_close(epoll_fd);
    return OE_OK;
}

/* This server acts as an echo server.  It accepts a connection,
 * receives messages, and echoes them back.
 */
int ecall_run_server()
{
    int status = OE_FAILURE;
    const static char TESTDATA[] = "This is TEST DATA\n";
    int listenfd = oe_socket(OE_AF_INET, OE_SOCK_STREAM, 0);
    int connfd = 0;
    struct oe_sockaddr_in serv_addr = {0};

    const int optVal = 1;
    const socklen_t optLen = sizeof(optVal);
    int rtn = oe_setsockopt(
        listenfd, OE_SOL_SOCKET, OE_SO_REUSEADDR, (void*)&optVal, optLen);
    if (rtn > 0)
    {
        printf("oe_setsockopt failed errno = %d\n", oe_errno);
    }

    serv_addr.sin_family = OE_AF_INET;
    serv_addr.sin_addr.s_addr = oe_htonl(OE_INADDR_LOOPBACK);
    serv_addr.sin_port = oe_htons(1493);

    printf("accepting\n");
    oe_bind(listenfd, (struct oe_sockaddr*)&serv_addr, sizeof(serv_addr));
    oe_listen(listenfd, 10);

    while (1)
    {
        oe_sleep(1);
        printf("accepting\n");
        connfd = oe_accept(listenfd, (struct oe_sockaddr*)NULL, NULL);
        if (connfd >= 0)
        {
            printf("accepted fd = %d\n", connfd);
            do
            {
                ssize_t n = oe_write(connfd, TESTDATA, strlen(TESTDATA));
                if (n > 0)
                {
                    printf("write test data n = %ld\n", n);
                    oe_close(connfd);
                    break;
                }
                else
                {
                    printf("write test data n = %ld errno = %d\n", n, oe_errno);
                }
                oe_sleep(3);
            } while (1);
            break;
        }
        else
        {
            printf("accept failed errno = %d \n", oe_errno);
        }
    }

    oe_close(listenfd);
    printf("exit from server thread\n");
    return status;
}

#define MAX_EVENTS 5
#define READ_SIZE 10

int oe_test_do_epoll()
{
    int running = 1, event_count, i;
    ssize_t bytes_read;
    char read_buffer[READ_SIZE + 1];
    struct oe_epoll_event event, events[MAX_EVENTS];
    int epoll_fd = oe_epoll_create1(0);

    if (epoll_fd == -1)
    {
        fprintf(stderr, "Failed to create epoll file descriptor\n");
        return 1;
    }

    event.events = OE_EPOLLIN;
    event.data.fd = 0;

    if (oe_epoll_ctl(epoll_fd, OE_EPOLL_CTL_ADD, 0, &event))
    {
        fprintf(stderr, "Failed to add file descriptor to epoll\n");
        oe_close(epoll_fd);
        return 1;
    }

    while (running)
    {
        printf("\nPolling for input...\n");
        event_count = oe_epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
        printf("%d ready events\n", event_count);
        for (i = 0; i < event_count; i++)
        {
            printf("Reading file descriptor '%d' -- ", events[i].data.fd);
            bytes_read = oe_read(events[i].data.fd, read_buffer, READ_SIZE);
            printf("%zd bytes read.\n", bytes_read);
            read_buffer[bytes_read] = '\0';
            printf("Read '%s'\n", read_buffer);

            if (!strncmp(read_buffer, "stop\n", 5))
                running = 0;
        }
    }

    if (oe_close(epoll_fd))
    {
        fprintf(stderr, "Failed to close epoll file descriptor\n");
        return 1;
    }
    return 0;
}
OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    256,  /* HeapPageCount */
    256,  /* StackPageCount */
    16);  /* TCSCount */
