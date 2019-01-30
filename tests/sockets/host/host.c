// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define OE_LIBC_SUPPRESS_DEPRECATIONS
#include <netinet/in.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_test_u.h"

#include <netinet/in.h>
#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket_test_u.h"

#define SERVER_PORT "12345"

void* enclave_server_thread(void* arg)
{
    oe_enclave_t* server_enclave = (oe_enclave_t*)arg;
    int retval = 0;
    OE_TEST(ecall_run_server(server_enclave, &retval) == OE_OK);
    return NULL;
}

void* host_server_thread(void* arg)
{
    static char TESTDATA[] = "This is TEST DATA\n";
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int connfd = 0;
    struct sockaddr_in serv_addr = {0};

    (void)arg;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(1492);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    // just once while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        write(connfd, TESTDATA, strlen(TESTDATA));
        printf("write test data\n");

        close(connfd);
        sleep(1);
    }

    printf("exit from server thread\n");
    return NULL;
}

char* host_client()

{
    int sockfd = 0;
    ssize_t n = 0;
    static char recvBuff[1024];
    struct sockaddr_in serv_addr = {0};

    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return NULL;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(1492);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return NULL;
    }

    while ((n = read(sockfd, recvBuff, sizeof(recvBuff) - 1)) > 0)
    {
        recvBuff[n] = 0;
    }

    if (n < 0)
    {
        printf("\n Read error \n");
    }

    return &recvBuff[0];
}

int main(int argc, const char* argv[])
{
    static char TESTDATA[] = "This is TEST DATA\n";
    // oe_result_t result;
    // oe_enclave_t* server_enclave = NULL;
    // oe_enclave_t* client_enclave = NULL;
    pthread_t server_thread_id = 0;
    // int ret = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH\n", argv[0]);
        return 1;
    }
    // disable buffering
    setvbuf(stdout, NULL, _IONBF, 0);

#if 0
    const uint32_t flags = oe_get_create_flags();

    result = oe_create_socket_test_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &server_enclave);
    OE_TEST(result == OE_OK);

    result = oe_create_socket_test_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &client_enclave);
    OE_TEST(result == OE_OK);
#endif

    // Launch host server on a separate thread
    OE_TEST(
        pthread_create(&server_thread_id, NULL, host_server_thread, NULL) == 0);

    sleep(3); // Give the server time to launch
    char* test_data = host_client();

    printf("received: %s\n", test_data);
    OE_TEST(strcmp(test_data, TESTDATA) == 0);

#if 0
    // Launch server on a separate thread
    OE_TEST(
        pthread_create(
            &server_thread_id, NULL, server_thread, server_enclave) == 0);
#endif

#if 0
    // Lauch client and talk to server
    OE_TEST(
        ecall_run_client(client_enclave, &ret, "localhost", SERVER_PORT) ==
        OE_OK);
#endif

    pthread_join(server_thread_id, NULL);

#if 0
    OE_TEST(oe_terminate_enclave(client_enclave) == OE_OK);
    OE_TEST(oe_terminate_enclave(server_enclave) == OE_OK);
#endif

    printf("=== passed all tests (socket_test)\n");

    return 0;
}
