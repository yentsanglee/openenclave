// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <mbedtls/platform.h>

#if !defined(MBEDTLS_BIGNUM_C)
#error "MBEDTLS_BIGNUM_C undefined"
#endif

#if !defined(MBEDTLS_ENTROPY_C)
#error "MBEDTLS_ENTROPY_C undefined"
#endif

#if !defined(MBEDTLS_SSL_TLS_C)
#error "MBEDTLS_SSL_TLS_C undefined"
#endif

#if !defined(MBEDTLS_SSL_CLI_C)
#error "MBEDTLS_SSL_CLI_C undefined"
#endif

#if !defined(MBEDTLS_NET_C)
#error "MBEDTLS_NET_C undefined"
#endif

#if !defined(MBEDTLS_RSA_C)
#error "MBEDTLS_RSA_C undefined"
#endif

#if !defined(MBEDTLS_CERTS_C)
#error "MBEDTLS_CERTS_C undefined"
#endif

#if !defined(MBEDTLS_PEM_PARSE_C)
#error "MBEDTLS_PEM_PARSE_C undefined"
#endif

#if !defined(MBEDTLS_CTR_DRBG_C)
#error "MBEDTLS_CTR_DRBG_C undefined"
#endif

#if !defined(MBEDTLS_X509_CRT_PARSE_C)
#error "MBEDTLS_X509_CRT_PARSE_C undefined"
#endif

#define main client_main

#pragma GCC diagnostic ignored "-Wsign-conversion"

#include "../../../3rdparty/mbedtls/mbedtls/programs/ssl/ssl_client1.c"
