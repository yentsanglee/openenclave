// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <resolv.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openenclave/host.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

#include "../common/common.h"

int verify_callback(int preverify_ok, X509_STORE_CTX* ctx);
int create_socket(char* server_name, char* server_port);

int parse_arguments(
    int argc,
    char** argv,
    char** server_name,
    char** server_port)
{
    int ret = 1;
    const char* option = NULL;
    int param_len = 0;

    if (argc != 3)
        goto print_usage;

    option = "-server:";
    param_len = strlen(option);
    if (strncmp(argv[1], option, param_len) != 0)
        goto print_usage;
    *server_name = (char*)(argv[1] + param_len);

    option = "-port:";
    param_len = strlen(option);
    if (strncmp(argv[2], option, param_len) != 0)
        goto print_usage;

    *server_port = (char*)(argv[2] + param_len);
    ret = 0;
    goto done;

print_usage:
    printf(TLS_CLIENT "Usage: %s -server:<name> -port:<port>\n", argv[0]);
done:
    return ret;
}

int ssl_read_all(SSL* ssl, uint8_t* buf, size_t size)
{
    int bytes_read = 0;
    int error;

    while (bytes_read < size)
    {
        int tmp = SSL_read(ssl, buf + bytes_read, size - bytes_read);
        if (tmp <= 0)
        {
            error = SSL_get_error(ssl, tmp);
            if (error == SSL_ERROR_WANT_READ)
                continue;
            printf(TLS_CLIENT "Failed! SSL_read returned %d\n", error);
            return -1;
        }
        bytes_read -= tmp;
    }

    return 0;
}

int ssl_write_all(SSL* ssl, const uint8_t* buf, size_t size)
{
    int bytes_written;
    int error;

    while ((bytes_written = SSL_write(ssl, buf, size)) <= 0)
    {
        error = SSL_get_error(ssl, bytes_written);
        if (error == SSL_ERROR_WANT_WRITE)
            continue;
        printf(TLS_CLIENT "Failed! SSL_write returned %d\n", error);
        return -1;
    }

    return 0;
}

int write_key_request(SSL* ssl)
{
    protocol_header hdr;

    printf(TLS_CLIENT "Write to server-->:\n\n");
    hdr.cmd = GETKEY;
    hdr.payload_size = 0;

    if (ssl_write_all(ssl, (unsigned char*)&hdr, sizeof(hdr)) != 0)
    {
        printf(TLS_CLIENT "Failed! ssl_write_all return an error.\n");
        return -1;
    }

    printf(TLS_CLIENT "\n\n%zu bytes written\n\n", sizeof(hdr));
    return 0;
}

int read_key(SSL* ssl, unsigned char* buf)
{
    protocol_header hdr;

    printf(TLS_CLIENT "\n\n<-- Read from server:\n");
    if (ssl_read_all(ssl, (unsigned char*)&hdr, sizeof(hdr)) != 0)
    {
        printf(TLS_CLIENT "Failed! ssl_read_all returned an error.\n");
        return -1;
    }

    if (ssl_read_all(ssl, buf, hdr.payload_size) != 0)
    {
        printf(TLS_CLIENT "Failed! ssl_read_all returned an error.\n");
        return -1;
    }

    return 0;
}

// This routine conducts a simple protocol for sending client encrypted data
// to the server. The protocol is the following:
//      1. Client sends public key request to the enclave server.
//      2. Enclave generates a private/public key pair and sends the public key
//      to the client.
//      3. Client encrypts the client encryption key with the public key and
//      sends the data + key to server.
//      4. Server uses the private key to decrypt the encryption key and then
//      uses that key to decrypt the data.
int communicate_with_server(SSL* ssl)
{
    unsigned char buf[4096];
    int ret = 1;

    // Step 1: Write public key request.
    ret = write_key_request(ssl);
    if (ret != 0)
    {
        printf(TLS_CLIENT "Failed! write_key_request returned an error.\n");
        goto done;
    }

    // Step 2: Client receives public key from server.
    ret = read_key(ssl, buf);
    if (ret != 0)
    {
        printf(TLS_CLIENT "Failed! read_key returned an error.\n");
        goto done;
    }

    printf("Printing out the public key: %s\n", buf);

    ret = 0;

done:
    return ret;
}

// create a socket and connect to the server_name:server_port
int create_socket(char* server_name, char* server_port)
{
    int sockfd = -1;
    char* addr_ptr = NULL;
    int port = 0;
    struct hostent* host = NULL;
    struct sockaddr_in dest_addr;

    if ((host = gethostbyname(server_name)) == NULL)
    {
        printf(TLS_CLIENT "Error: Cannot resolve hostname %s.\n", server_name);
        goto done;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf(TLS_CLIENT "Error: Cannot create socket %d.\n", errno);
        goto done;
    }

    port = atoi(server_port);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);

    memset(&(dest_addr.sin_zero), '\0', 8);
    addr_ptr = inet_ntoa(dest_addr.sin_addr);

    if (connect(
            sockfd, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr)) ==
        -1)
    {
        printf(
            TLS_CLIENT "failed to connect to %s:%s (errno=%d)\n",
            server_port,
            server_port,
            errno);
        close(sockfd);
        sockfd = -1;
        goto done;
    }
    printf(TLS_CLIENT "connected to %s:%s\n", server_name, server_port);
done:
    return sockfd;
}

int main(int argc, char** argv)
{
    int ret = 1;
    X509* cert = NULL;
    SSL_CTX* ctx = NULL;
    SSL* ssl = NULL;
    int serversocket = 0;
    char* server_name = NULL;
    char* server_port = NULL;
    int error = 0;

    printf("\nStarting" TLS_CLIENT "\n\n\n");
    if ((error = parse_arguments(argc, argv, &server_name, &server_port)) != 0)
    {
        printf(
            TLS_CLIENT "TLS client:parse input parmeter failed (%d)!\n", error);
        goto done;
    }

    // initialize openssl library and register algorithms
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();

    if (SSL_library_init() < 0)
    {
        printf(TLS_CLIENT
               "TLS client: could not initialize the OpenSSL library !\n");
        goto done;
    }

    if ((ctx = SSL_CTX_new(SSLv23_client_method())) == NULL)
    {
        printf(TLS_CLIENT "TLS client: unable to create a new SSL context\n");
        goto done;
    }

    // choose TLSv1.2 by excluding SSLv2, SSLv3 ,TLS 1.0 and TLS 1.1
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1);
    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_1);

    // specify the verify_callback for custom verification
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, &verify_callback);

    if ((ssl = SSL_new(ctx)) == NULL)
    {
        printf(TLS_CLIENT
               "Unable to create a new SSL connection state object\n");
        goto done;
    }

    serversocket = create_socket(server_name, server_port);
    if (serversocket == -1)
    {
        printf(
            TLS_CLIENT
            "create a socket and initate a TCP connect to server: %s:%s "
            "(errno=%d)\n",
            server_name,
            server_port,
            errno);
        goto done;
    }

    // setup ssl socket and initiate TLS connection with TLS server
    SSL_set_fd(ssl, serversocket);
    if ((error = SSL_connect(ssl)) != 1)
    {
        printf(
            TLS_CLIENT "Error: Could not establish an SSL session ret2=%d "
                       "SSL_get_error()=%d\n",
            error,
            SSL_get_error(ssl, error));
        goto done;
    }
    printf(
        TLS_CLIENT "successfully established TLS channel:%s\n",
        SSL_get_version(ssl));

    // start the client server communication
    if ((error = communicate_with_server(ssl)) != 0)
    {
        printf(TLS_CLIENT "Failed: communicate_with_server (ret=%d)\n", error);
        goto done;
    }

    // Free the structures we don't need anymore
    if (serversocket != -1)
        close(serversocket);

    ret = 0;
done:
    if (ssl)
        SSL_free(ssl);

    if (cert)
        X509_free(cert);

    if (ctx)
        SSL_CTX_free(ctx);

    printf(TLS_CLIENT " %s\n", (ret == 0) ? "success" : "failed");
    return (ret);
}
