/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */

#include <openenclave/enclave.h>

// enclave.h must come before socket.h
#include <openenclave/internal/device.h>
#include <openenclave/internal/host_socket.h>
#include <openenclave/internal/netinet/in.h>
#include <openenclave/internal/sockaddr.h>
#include <openenclave/internal/socket.h>

#include <socket_test_t.h>
#include <stdio.h>
#include <string.h>

int ecall_device_init()
{
    (void)oe_allocate_devid(OE_DEVICE_ID_HOST_SOCKET);
    (void)oe_set_devid_device(
        OE_DEVICE_ID_HOST_SOCKET, oe_socket_get_hostsock());
    return 0;
}

/* This client connects to an echo server, sends a text message,
 * and outputs the text reply.
 */
int ecall_run_client(char* recv_buff, ssize_t* recv_buff_len)
{
    oe_sockfd_t sockfd = 0;
    ssize_t n = 0;
    size_t buff_len = (size_t)*recv_buff_len;
    struct oe_sockaddr_in serv_addr = {0};

    memset(recv_buff, '0', buff_len);
    printf("create socket\n");
    if ((sockfd = oe_socket(OE_AF_INET, OE_SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return OE_FAILURE;
    }
    serv_addr.sin_family = OE_AF_INET;
    serv_addr.sin_addr.s_addr = oe_htonl(OE_INADDR_LOOPBACK);
    serv_addr.sin_port = oe_htons(1492);

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

    printf("reading...\n");
    n = oe_read(sockfd, recv_buff, buff_len);
    *recv_buff_len = n;
    if (n > 0)
    {
        printf("finished reading: %ld bytes...\n", n);
    }
    else
    {
        printf("Read error, Fail\n");
        oe_close(sockfd);
        return OE_FAILURE;
    }

    oe_close(sockfd);
    return OE_OK;
}

/* This server acts as an echo server.  It accepts a connection,
 * receives messages, and echoes them back.
 */
int ecall_run_server()
{
    int status = OE_FAILURE;
#if 0
    struct addrinfo* ai = NULL;
    SOCKET listener = INVALID_SOCKET;
    SOCKET s = INVALID_SOCKET;

    /* Resolve service name. */
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int err = getaddrinfo(NULL, serv, &hints, &ai);
    if (err != 0)
    {
        goto Done;
    }

    /* Create listener socket. */
    listener = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (listener == INVALID_SOCKET)
    {
        goto Done;
    }
    if (bind(listener, ai->ai_addr, (int)ai->ai_addrlen) == SOCKET_ERROR)
    {
        goto Done;
    }
    if (listen(listener, SOMAXCONN) == SOCKET_ERROR)
    {
        goto Done;
    }
    printf("Listening on %s...\n", serv);

    /* Accept a client connection. */
    struct sockaddr_storage addr;
    int addrlen = sizeof(addr);
    s = accept(listener, (struct sockaddr*)&addr, &addrlen);
    if (s == INVALID_SOCKET)
    {
        goto Done;
    }

    /* Receive a text message, prefixed by its size. */
    uint32_t netMessageLength;
    uint32_t messageLength;
    char message[80];
    ssize_t bytesReceived = recv(
        s, (char*)&netMessageLength, sizeof(netMessageLength), MSG_WAITALL);
    if (bytesReceived == SOCKET_ERROR)
    {
        goto Done;
    }
    messageLength = ntohl(netMessageLength);
    if (messageLength > sizeof(message))
    {
        goto Done;
    }
    bytesReceived = recv(s, message, messageLength, MSG_WAITALL);
    if (bytesReceived != messageLength)
    {
        goto Done;
    }

    /* Send it back to the client, prefixed by its size. */
    int bytesSent =
        send(s, (char*)&netMessageLength, sizeof(netMessageLength), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        goto Done;
    }
    bytesSent = send(s, message, (int)messageLength, 0);
    if (bytesSent == SOCKET_ERROR)
    {
        goto Done;
    }
    status = OE_OK;

Done:
    if (s != INVALID_SOCKET)
    {
        printf("Server closing socket\n");
        closesocket(s);
    }
    if (listener != INVALID_SOCKET)
    {
        printf("Server closing listener socket\n");
        closesocket(listener);
    }
    if (ai != NULL)
    {
        freeaddrinfo(ai);
    }
#endif
    return status;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    256,  /* HeapPageCount */
    256,  /* StackPageCount */
    1);   /* TCSCount */
