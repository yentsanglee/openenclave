// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _OE_ENCLAVE_CRL_H
#define _OE_ENCLAVE_CRL_H

#define OE_NEED_STDC_DEFINES
#include <mbedtls/x509_crl.h>
#include <openenclave/corelibc/bits/undefs.h>

#include <openenclave/internal/crl.h>

typedef struct _crl
{
    uint64_t magic;
    mbedtls_x509_crl* crl;
} crl_t;

bool crl_is_valid(const crl_t* impl);

#endif /* _OE_ENCLAVE_CRL_H */
