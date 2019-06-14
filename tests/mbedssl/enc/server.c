// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <mbedtls/platform.h>

#define main server_main

#pragma GCC diagnostic ignored "-Wsign-conversion"

#if !defined(MBEDTLS_BIGNUM_C)
#error "MBEDTLS_BIGNUM_C disabled"
#endif

#if !defined(MBEDTLS_CERTS_C)
#error "MBEDTLS_CERTS_C undefined"
#endif

#if !defined(MBEDTLS_ENTROPY_C)
#error "MBEDTLS_ENTROPY_C undefined"
#endif

#if !defined(MBEDTLS_SSL_TLS_C)
#error "MBEDTLS_SSL_TLS_C undefined"
#endif

#if !defined(MBEDTLS_SSL_SRV_C)
#error "MBEDTLS_SSL_SRV_C undefined"
#endif

#if !defined(MBEDTLS_NET_C)
#error "MBEDTLS_NET_C undefined"
#endif

#if !defined(MBEDTLS_RSA_C)
#error "MBEDTLS_RSA_C undefined"
#endif

#if !defined(MBEDTLS_CTR_DRBG_C)
#error "MBEDTLS_CTR_DRBG_C undefined"
#endif

#if !defined(MBEDTLS_X509_CRT_PARSE_C)
#error "MBEDTLS_X509_CRT_PARSE_C undefined"
#endif

#if !defined(MBEDTLS_FS_IO)
#error "MBEDTLS_FS_IO undefined"
#endif

#if !defined(MBEDTLS_PEM_PARSE_C)
#error "MBEDTLS_PEM_PARSE_C undefined"
#endif

#include "../../../3rdparty/mbedtls/mbedtls/programs/ssl/ssl_server.c"
