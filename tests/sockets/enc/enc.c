/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */
#include <openenclave/enclave.h>

// enclave.h must come before socket.h
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <socket_test_t.h>
#include <stdio.h>
#include <string.h>

/* This client connects to an echo server, sends a text message,
 * and outputs the text reply.
 */
int ecall_run_client(char* server, char* serv)
{
    int status = OE_FAILURE;
    struct addrinfo* ai = NULL;
    int s = -1;

    printf("Connecting to %s %s...\n", server, serv);

    /* Resolve server name. */
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int err = getaddrinfo(server, serv, &hints, &ai);
    if (err != 0)
    {
        goto Done;
    }

    /* Create connection. */
    s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (s == -1)
    {
        goto Done;
    }
    if (connect(s, ai->ai_addr, (socklen_t) ai->ai_addrlen) == -1)
    {
        goto Done;
    }

    /* Send a message, prefixed by its size. */
    const char* message = "Hello, world!";
    printf("Sending message: %s\n", message);
    size_t messageLength = strlen(message);
    uint32_t netMessageLength = htonl((uint32_t) messageLength);
    ssize_t bytesSent =
        send(s, (char*)&netMessageLength, (int) sizeof(netMessageLength), 0);
    if (bytesSent == -1)
    {
        goto Done;
    }
    bytesSent = send(s, message, (size_t) messageLength, 0);
    if (bytesSent == -1)
    {
        goto Done;
    }

    /* Receive a text reply, prefixed by its size. */
    uint32_t replyLength;
    char reply[80];
    ssize_t bytesReceived =
        recv(s, (char*)&replyLength, sizeof(replyLength), MSG_WAITALL);
    if (bytesReceived == -1)
    {
        goto Done;
    }
    replyLength =  ntohl(replyLength);
    if (replyLength > sizeof(reply) - 1)
    {
        goto Done;
    }
    bytesReceived = recv(s, reply, replyLength, MSG_WAITALL);
    if (bytesReceived != bytesSent)
    {
        goto Done;
    }

    /* Add null termination. */
    reply[replyLength] = 0;

    /* Print the reply. */
    printf("Received reply: %s\n", reply);

    status = OE_OK;

Done:
    if (s != -1)
    {
        printf("Client closing socket\n");        
        close(s);
    }
    if (ai != NULL)
    {
        freeaddrinfo(ai);
    }
    return status;
}

/* This server acts as an echo server.  It accepts a connection,
 * receives messages, and echoes them back.
 */
int ecall_run_server(char* serv)
{
    int status = OE_FAILURE;
    struct addrinfo* ai = NULL;
    int listener = -1;
    int s = -1;

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
    if (listener == -1)
    {
        goto Done;
    }
    if (bind(listener, ai->ai_addr, (unsigned int) ai->ai_addrlen) == -1)
    {
        goto Done;
    }
    if (listen(listener, SOMAXCONN) == -1)
    {
        goto Done;
    }
    printf("Listening on %s...\n", serv);

    /* Accept a client connection. */
    struct sockaddr_storage addr;
    unsigned int addrlen = sizeof(addr);
    s = accept(listener, (struct sockaddr*)&addr, &addrlen);
    if (s == -1)
    {
        goto Done;
    }

    /* Receive a text message, prefixed by its size. */
    uint32_t netMessageLength;
    uint32_t messageLength;
    char message[80];
    ssize_t bytesReceived = recv(
        s, (char*)&netMessageLength, sizeof(netMessageLength), MSG_WAITALL);
    if (bytesReceived == -1)
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
    ssize_t bytesSent =
        send(s, (char*)&netMessageLength, sizeof(netMessageLength), 0);
    if (bytesSent == -1)
    {
        goto Done;
    }
    bytesSent = send(s, message, messageLength, 0);
    if (bytesSent == -1)
    {
        goto Done;
    }
    status = OE_OK;

Done:
    if (s != -1)
    {
        printf("Server closing socket\n");
        close(s);
    }
    if (listener != -1)
    {
        printf("Server closing listener socket\n");
        close(listener);
    }
    if (ai != NULL)
    {
        freeaddrinfo(ai);
    }
    return status;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    256,  /* HeapPageCount */
    256,  /* StackPageCount */
    1);   /* TCSCount */
