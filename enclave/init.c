// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "init.h"
#include <openssl/err.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wbitwise-op-parentheses"
#pragma GCC diagnostic ignored "-Wshift-op-parentheses"
#pragma GCC diagnostic ignored "-Wconversion"
#endif /* __GNUC__ */
#include <openssl/pem.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /* __GNUC__ */
#include <pthread.h>

static pthread_once_t _once = PTHREAD_ONCE_INIT;

static void _initialize(void)
{
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
}

void oe_initialize_openssl(void)
{
    pthread_once(&_once, _initialize);
}
