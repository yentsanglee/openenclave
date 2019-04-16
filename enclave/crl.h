// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_HOST_CRYPTO_CRL_H
#define _OE_HOST_CRYPTO_CRL_H

#include <openenclave/internal/crl.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wbitwise-op-parentheses"
#pragma GCC diagnostic ignored "-Wshift-op-parentheses"
#pragma GCC diagnostic ignored "-Wconversion"
#endif /* __GNUC__ */
#include <openssl/x509.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /* __GNUC__ */

typedef struct _crl
{
    uint64_t magic;
    X509_CRL* crl;
} crl_t;

bool crl_is_valid(const crl_t* impl);

#endif /* _OE_HOST_CRYPTO_CRL_H */
