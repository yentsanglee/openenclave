// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/pk.h>
#include <openenclave/internal/csr.h>
#include <openenclave/internal/ec.h>
#include <openenclave/internal/enclavelibc.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include "key.h"
#include "random.h"

oe_result_t oe_csr_from_ec_key(
    oe_ec_private_key_t* private_key_,
    oe_ec_public_key_t* public_key_,
    const char* name,
    uint8_t** csr,
    size_t* csr_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_private_key_t* private_key = (oe_private_key_t*) private_key_;
    oe_public_key_t* public_key = (oe_public_key_t*) public_key_;
    mbedtls_x509write_csr csr_ctx;
    mbedtls_ctr_drbg_context* drbg;
    uint8_t* csr_local = NULL;

    mbedtls_x509write_csr_init(&csr_ctx);

    if (!public_key || !private_key || !name || !csr || !csr_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* The public key of the CSR should match the signature private key. */
    if (mbedtls_pk_check_pair(&public_key->pk, &private_key->pk) != 0)
        OE_RAISE(OE_CONSTRAINT_FAILED);

    if (mbedtls_x509write_csr_set_subject_name(&csr_ctx, name) != 0)
        OE_RAISE(OE_FAILURE);

   mbedtls_x509write_csr_set_md_alg(&csr_ctx, MBEDTLS_MD_SHA256);

    /* Set the CSR key. We pick the private, since it should have both keys. */
    mbedtls_x509write_csr_set_key(&csr_ctx, &private_key->pk);

    if (!(drbg = oe_mbedtls_get_drbg()))
        OE_RAISE(OE_FAILURE);

    /*
     * Allocate a large enough buffer. The API unfortunately doesn't say how
     * big of a buffer, so we hope this is enough.
     */
    if (!(csr_local = (uint8_t*) oe_calloc(4096, 1)))
        OE_RAISE(OE_FAILURE);
    
    /* Sign and export the key. */
    if (mbedtls_x509write_csr_pem(
            &csr_ctx, csr_local, 4096, mbedtls_ctr_drbg_random, drbg) != 0)
        OE_RAISE(OE_FAILURE);

    /* PEM files are null terminated, so we can find the true length */
    *csr = csr_local;
    *csr_size = oe_strlen((const char*) csr_local);
    result = OE_OK;
    csr_local = NULL;

done:
    mbedtls_x509write_csr_free(&csr_ctx);
    return result;

}

