// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define OE_LIBC_SUPPRESS_DEPRECATIONS
#include <netinet/in.h>
#include <openenclave/internal/tests.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "epoll_test_u.h"

#define SERVER_PORT "12345"

void oe_socket_install_hostsock(void);
void oe_epoll_install_hostepoll(void);

void* host_server_thread(void* arg)
{
    const static char TESTDATA[] = "This is TEST DATA\n";
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int connfd = 0;
    struct sockaddr_in serv_addr = {0};

    (void)arg;
    const int optVal = 1;
    const socklen_t optLen = sizeof(optVal);

    int rtn =
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&optVal, optLen);
    if (rtn > 0)
    {
        printf("setsockopt failed errno = %d\n", errno);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(1492);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    int n = 0;
    while (1)
    {
        printf("accepting\n");
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        printf("accepted fd = %d\n", connfd);
        if (connfd >= 0)
        {
            write(connfd, TESTDATA, strlen(TESTDATA));
            printf("write test data\n");
            close(connfd);
            if (n++ > 10)
                break;
        }
        sleep(5);
    }

    close(listenfd);
    printf("exit from server thread\n");
    return NULL;
}

int main(int argc, const char* argv[])
{
    //    static char TESTDATA[] = "This is TEST DATA\n";
    oe_result_t result;
    oe_enclave_t* client_enclave = NULL;
    pthread_t server_thread_id = 0;
    int ret = 0;
    char test_data_rtn[1024] = {0};
    ssize_t test_data_len = 1024;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH\n", argv[0]);
        return 1;
    }
    // disable buffering
    setvbuf(stdout, NULL, _IONBF, 0);

#if 0
    // host server to host client
    OE_TEST(
        pthread_create(&server_thread_id, NULL, host_server_thread, NULL) == 0);

    sleep(3); // Give the server time to launch
    char* test_data = host_client();

    printf("received: %s\n", test_data);
    OE_TEST(strcmp(test_data, TESTDATA) == 0);

    pthread_join(server_thread_id, NULL);

    sleep(3); // Let the net stack settle
#endif

    // host server to enclave client
    OE_TEST(
        pthread_create(&server_thread_id, NULL, host_server_thread, NULL) == 0);

    sleep(3); // Give the server time to launch
    const uint32_t flags = oe_get_create_flags();

    // oe_fs_install_hostfs();
    oe_socket_install_hostsock();
    oe_epoll_install_hostepoll();

    result = oe_create_epoll_test_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &client_enclave);

    OE_TEST(result == OE_OK);

    OE_TEST(ecall_device_init(client_enclave, &ret) == OE_OK);

    test_data_len = 1024;
    OE_TEST(ecall_run_client(client_enclave, &ret) == OE_OK);

    printf("host received: %s\n", test_data_rtn);

    pthread_join(server_thread_id, NULL);
    OE_TEST(oe_terminate_enclave(client_enclave) == OE_OK);

    printf("=== passed all tests (socket_test)\n");

    return 0;
}
