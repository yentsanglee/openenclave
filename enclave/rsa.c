// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "rsa.h"
#include <openenclave/bits/safecrt.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include "key.h"
#include "pem.h"
#include "random.h"

static uint64_t _PRIVATE_KEY_MAGIC = 0xd48de5bae3994b41;
static uint64_t _PUBLIC_KEY_MAGIC = 0x713600af058c447a;

OE_STATIC_ASSERT(sizeof(oe_private_key_t) <= sizeof(oe_rsa_private_key_t));
OE_STATIC_ASSERT(sizeof(oe_public_key_t) <= sizeof(oe_rsa_public_key_t));

static oe_result_t _copy_key(
    mbedtls_pk_context* dest,
    const mbedtls_pk_context* src,
    bool copy_private_fields)
{
    oe_result_t result = OE_UNEXPECTED;
    const mbedtls_pk_info_t* info = NULL;
    mbedtls_rsa_context* rsa = NULL;
    int rc = 0;

    if (dest)
        mbedtls_pk_init(dest);

    /* Check for invalid parameters */
    if (!dest || !src)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Lookup the RSA info */
    if (!(info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)))
        OE_RAISE(OE_PUBLIC_KEY_NOT_FOUND);

    /* If not an RSA key */
    if (src->pk_info != info)
        OE_RAISE(OE_FAILURE);

    /* Setup the context for this key type */
    rc = mbedtls_pk_setup(dest, info);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Get the context for this key type */
    if (!(rsa = dest->pk_ctx))
        OE_RAISE(OE_FAILURE);

    /* Initialize the RSA key from the source */
    if (mbedtls_rsa_copy(rsa, mbedtls_pk_rsa(*src)) != 0)
        OE_RAISE(OE_FAILURE);

    /* If not a private key, then clear private key fields */
    if (!copy_private_fields)
    {
        mbedtls_mpi_free(&rsa->D);
        mbedtls_mpi_free(&rsa->P);
        mbedtls_mpi_free(&rsa->Q);
        mbedtls_mpi_free(&rsa->DP);
        mbedtls_mpi_free(&rsa->DQ);
        mbedtls_mpi_free(&rsa->QP);
        mbedtls_mpi_free(&rsa->RN);
        mbedtls_mpi_free(&rsa->RP);
        mbedtls_mpi_free(&rsa->RQ);
        mbedtls_mpi_free(&rsa->Vi);
        mbedtls_mpi_free(&rsa->Vf);
    }

    result = OE_OK;

done:

    if (result != OE_OK)
        mbedtls_pk_free(dest);

    return result;
}

static oe_result_t _get_public_key_modulus_or_exponent(
    const oe_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size,
    bool get_modulus)
{
    oe_result_t result = OE_UNEXPECTED;
    size_t required_size;
    mbedtls_rsa_context* rsa;
    const mbedtls_mpi* mpi;

    /* Check for invalid parameters */
    if (!oe_public_key_is_valid(public_key, _PUBLIC_KEY_MAGIC) || !buffer_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* If buffer is null, then buffer_size must be zero */
    if (!buffer && *buffer_size != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the RSA context */
    if (!(rsa = public_key->pk.pk_ctx))
        OE_RAISE(OE_FAILURE);

    /* Pick modulus or exponent */
    if (!(mpi = get_modulus ? &rsa->N : &rsa->E))
        OE_RAISE(OE_FAILURE);

    /* Determine the required size in bytes */
    required_size = mbedtls_mpi_size(mpi);

    /* If buffer is null or not big enough */
    if (!buffer || (*buffer_size < required_size))
    {
        *buffer_size = required_size;
        OE_RAISE(OE_BUFFER_TOO_SMALL);
    }

    /* Copy key bytes to the caller's buffer */
    if (mbedtls_mpi_write_binary(mpi, buffer, required_size) != 0)
        OE_RAISE(OE_FAILURE);

    *buffer_size = required_size;

    result = OE_OK;

done:

    return result;
}

static oe_result_t _generate_key_pair(
    uint64_t bits,
    uint64_t exponent,
    oe_private_key_t* private_key,
    oe_public_key_t* public_key)
{
    oe_result_t result = OE_UNEXPECTED;
    mbedtls_ctr_drbg_context* drbg = NULL;
    mbedtls_pk_context pk;
    int rc = 0;

    /* Initialize structures */
    mbedtls_pk_init(&pk);

    if (private_key)
        oe_secure_zero_fill(private_key, sizeof(*private_key));

    if (public_key)
        oe_secure_zero_fill(public_key, sizeof(*public_key));

    /* Check for invalid parameters */
    if (!private_key || !public_key)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Check range of bits and exponent parameters */
    if (bits > OE_UINT_MAX || exponent > OE_INT_MAX)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the random number generator */
    if (!(drbg = oe_mbedtls_get_drbg()))
        OE_RAISE(OE_FAILURE);

    /* Create key struct */
    rc = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Generate the RSA key */
    rc = mbedtls_rsa_gen_key(
        mbedtls_pk_rsa(pk),
        mbedtls_ctr_drbg_random,
        drbg,
        (unsigned int)bits,
        (int)exponent);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Initialize the private key parameter */
    OE_CHECK(
        oe_private_key_init(private_key, &pk, _copy_key, _PRIVATE_KEY_MAGIC));

    /* Initialize the public key parameter */
    OE_CHECK(oe_public_key_init(public_key, &pk, _copy_key, _PUBLIC_KEY_MAGIC));

    result = OE_OK;

done:

    mbedtls_pk_free(&pk);

    if (result != OE_OK)
    {
        if (oe_private_key_is_valid(private_key, _PRIVATE_KEY_MAGIC))
            oe_private_key_free(private_key, _PRIVATE_KEY_MAGIC);

        if (oe_public_key_is_valid(public_key, _PUBLIC_KEY_MAGIC))
            oe_public_key_free(public_key, _PUBLIC_KEY_MAGIC);
    }

    return result;
}

oe_result_t oe_public_key_get_modulus(
    const oe_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size)
{
    return _get_public_key_modulus_or_exponent(
        public_key, buffer, buffer_size, true);
}

oe_result_t oe_public_key_get_exponent(
    const oe_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size)
{
    return _get_public_key_modulus_or_exponent(
        public_key, buffer, buffer_size, false);
}

static oe_result_t oe_public_key_equal(
    const oe_public_key_t* public_key1,
    const oe_public_key_t* public_key2,
    bool* equal)
{
    oe_result_t result = OE_UNEXPECTED;

    if (equal)
        *equal = false;

    /* Reject bad parameters */
    if (!oe_public_key_is_valid(public_key1, _PUBLIC_KEY_MAGIC) ||
        !oe_public_key_is_valid(public_key2, _PUBLIC_KEY_MAGIC) || !equal)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Compare the exponent and modulus */
    {
        const mbedtls_rsa_context* rsa1 = public_key1->pk.pk_ctx;
        const mbedtls_rsa_context* rsa2 = public_key2->pk.pk_ctx;

        if (!rsa1 || !rsa2)
            OE_RAISE(OE_INVALID_PARAMETER);

        if (mbedtls_mpi_cmp_mpi(&rsa1->N, &rsa2->N) == 0 &&
            mbedtls_mpi_cmp_mpi(&rsa1->E, &rsa2->E) == 0)
        {
            *equal = true;
        }
    }

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_rsa_public_key_init(
    oe_rsa_public_key_t* public_key,
    const mbedtls_pk_context* pk)
{
    return oe_public_key_init(
        (oe_public_key_t*)public_key, pk, _copy_key, _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_private_key_read_pem(
    oe_rsa_private_key_t* private_key,
    const uint8_t* pem_data,
    size_t pem_size)
{
    return oe_private_key_read_pem(
        pem_data,
        pem_size,
        (oe_private_key_t*)private_key,
        MBEDTLS_PK_RSA,
        _PRIVATE_KEY_MAGIC);
}

oe_result_t oe_rsa_private_key_write_pem(
    const oe_rsa_private_key_t* private_key,
    uint8_t* pem_data,
    size_t* pem_size)
{
    return oe_private_key_write_pem(
        (const oe_private_key_t*)private_key,
        pem_data,
        pem_size,
        _PRIVATE_KEY_MAGIC);
}

oe_result_t oe_rsa_public_key_read_pem(
    oe_rsa_public_key_t* private_key,
    const uint8_t* pem_data,
    size_t pem_size)
{
    return oe_public_key_read_pem(
        pem_data,
        pem_size,
        (oe_public_key_t*)private_key,
        MBEDTLS_PK_RSA,
        _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_public_key_write_pem(
    const oe_rsa_public_key_t* private_key,
    uint8_t* pem_data,
    size_t* pem_size)
{
    return oe_public_key_write_pem(
        (const oe_public_key_t*)private_key,
        pem_data,
        pem_size,
        _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_private_key_free(oe_rsa_private_key_t* private_key)
{
    return oe_private_key_free(
        (oe_private_key_t*)private_key, _PRIVATE_KEY_MAGIC);
}

oe_result_t oe_rsa_public_key_free(oe_rsa_public_key_t* public_key)
{
    return oe_public_key_free((oe_public_key_t*)public_key, _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_private_key_sign(
    const oe_rsa_private_key_t* private_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    uint8_t* signature,
    size_t* signature_size)
{
    return oe_private_key_sign(
        (oe_private_key_t*)private_key,
        hash_type,
        hash_data,
        hash_size,
        signature,
        signature_size,
        _PRIVATE_KEY_MAGIC);
}

oe_result_t oe_rsa_public_key_verify(
    const oe_rsa_public_key_t* public_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    const uint8_t* signature,
    size_t signature_size)
{
    return oe_public_key_verify(
        (oe_public_key_t*)public_key,
        hash_type,
        hash_data,
        hash_size,
        signature,
        signature_size,
        _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_generate_key_pair(
    uint64_t bits,
    uint64_t exponent,
    oe_rsa_private_key_t* private_key,
    oe_rsa_public_key_t* public_key)
{
    return _generate_key_pair(
        bits,
        exponent,
        (oe_private_key_t*)private_key,
        (oe_public_key_t*)public_key);
}

oe_result_t oe_rsa_public_key_get_modulus(
    const oe_rsa_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size)
{
    return oe_public_key_get_modulus(
        (oe_public_key_t*)public_key, buffer, buffer_size);
}

oe_result_t oe_rsa_public_key_get_exponent(
    const oe_rsa_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size)
{
    return oe_public_key_get_exponent(
        (oe_public_key_t*)public_key, buffer, buffer_size);
}

oe_result_t oe_rsa_public_key_equal(
    const oe_rsa_public_key_t* public_key1,
    const oe_rsa_public_key_t* public_key2,
    bool* equal)
{
    return oe_public_key_equal(
        (oe_public_key_t*)public_key1, (oe_public_key_t*)public_key2, equal);
}

oe_result_t oe_rsa_public_key_encrypt(
    const oe_rsa_public_key_t* public_key,
    const uint8_t* plain,
    size_t plain_size,
    uint8_t* cipher,
    size_t* cipher_size)
{
    oe_result_t result = OE_UNEXPECTED;
    mbedtls_rsa_context* rsa;
    mbedtls_ctr_drbg_context* drbg = NULL;
    int res;

    /* Check valid parameters. */
    if (!public_key || !plain || !cipher || !cipher_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (!oe_public_key_is_valid(
            (oe_public_key_t*)public_key, _PUBLIC_KEY_MAGIC))
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the RSA context */
    if (!(rsa = mbedtls_pk_rsa(((oe_public_key_t*)public_key)->pk)))
        OE_RAISE(OE_FAILURE);

    /* Check if cipher size is large enough, */
    if (*cipher_size < rsa->len)
    {
        *cipher_size = rsa->len;
        OE_RAISE(OE_BUFFER_TOO_SMALL);
    }

    /* Get the random number generator */
    if (!(drbg = oe_mbedtls_get_drbg()))
        OE_RAISE(OE_FAILURE);

    /* Encypt the data. */
    mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    res = mbedtls_rsa_rsaes_oaep_encrypt(
        rsa,
        mbedtls_ctr_drbg_random,
        drbg,
        MBEDTLS_RSA_PUBLIC,
        NULL,
        0,
        plain_size,
        plain,
        cipher);
    if (res != 0)
        OE_RAISE(OE_FAILURE);

    mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, 0);
    result = OE_OK;

done:
    return result;
}

oe_result_t oe_rsa_private_key_decrypt(
    const oe_rsa_private_key_t* private_key,
    const uint8_t* cipher,
    size_t cipher_size,
    uint8_t* plain,
    size_t* plain_size)
{
    oe_result_t result = OE_UNEXPECTED;
    mbedtls_rsa_context* rsa;
    mbedtls_ctr_drbg_context* drbg = NULL;
    int res;

    /* Check valid parameters. */
    if (!private_key || !cipher || !plain || !plain_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (!oe_private_key_is_valid(
            (oe_private_key_t*)private_key, _PRIVATE_KEY_MAGIC))
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the RSA context */
    if (!(rsa = mbedtls_pk_rsa(((oe_private_key_t*)private_key)->pk)))
        OE_RAISE(OE_FAILURE);

    /* Check if the ciphertext size is the correct length. */
    if (cipher_size != rsa->len)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Check if plaintext size is large enough, */
    if (*plain_size < rsa->len)
    {
        *plain_size = rsa->len;
        OE_RAISE(OE_BUFFER_TOO_SMALL);
    }

    /* Get the random number generator */
    if (!(drbg = oe_mbedtls_get_drbg()))
        OE_RAISE(OE_FAILURE);

    /* Decrypt the data. */
    mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256);
    res = mbedtls_rsa_rsaes_oaep_decrypt(
        rsa,
        mbedtls_ctr_drbg_random,
        drbg,
        MBEDTLS_RSA_PRIVATE,
        NULL,
        0,
        plain_size,
        cipher,
        plain,
        *plain_size);
    if (res != 0)
        OE_RAISE(OE_FAILURE);

    mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, 0);
    result = OE_OK;

done:
    return result;
}

oe_result_t oe_rsa_private_key_get_parameters(
    const oe_rsa_private_key_t* private_key,
    uint8_t* m,
    size_t* m_size,
    uint8_t* p,
    size_t* p_size,
    uint8_t* q,
    size_t* q_size,
    uint8_t* d,
    size_t* d_size,
    uint8_t* e,
    size_t* e_size)
{
    oe_result_t result = OE_UNEXPECTED;
    mbedtls_rsa_context* rsa;
    int res;

    if (!private_key || !m_size || !p_size || !q_size || !d_size || !e_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (!oe_private_key_is_valid(
            (oe_private_key_t*)private_key, _PRIVATE_KEY_MAGIC))
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the RSA context */
    if (!(rsa = mbedtls_pk_rsa(((oe_private_key_t*)private_key)->pk)))
        OE_RAISE(OE_FAILURE);

    res = mbedtls_rsa_export_raw(
        rsa, m, *m_size, p, *p_size, q, *q_size, d, *d_size, e, *e_size);
    if (res != 0)
        OE_RAISE(OE_FAILURE);

    *m_size = rsa->len;
    *p_size = mbedtls_mpi_size(&rsa->P);
    *q_size = mbedtls_mpi_size(&rsa->Q);
    *d_size = mbedtls_mpi_size(&rsa->D);
    *e_size = mbedtls_mpi_size(&rsa->E);
    result = OE_OK;

done:
    return result;
}

static oe_result_t _oe_export_mpi_param(
    const mbedtls_mpi* num,
    uint8_t* buf,
    size_t* buf_size)
{
    oe_result_t result = OE_UNEXPECTED;

    if (buf)
    {
        if (*buf_size < mbedtls_mpi_size(num))
        {
            *buf_size = mbedtls_mpi_size(num);
            OE_RAISE(OE_BUFFER_TOO_SMALL);
        }

        if (mbedtls_mpi_write_binary(num, buf, *buf_size) != 0)
            OE_RAISE(OE_FAILURE);

        *buf_size = mbedtls_mpi_size(num);
    }

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_rsa_private_key_get_crt_parameters(
    const oe_rsa_private_key_t* private_key,
    uint8_t* dp,
    size_t* dp_size,
    uint8_t* dq,
    size_t* dq_size,
    uint8_t* qinv,
    size_t* qinv_size)
{
    oe_result_t result = OE_UNEXPECTED;
    mbedtls_rsa_context* rsa;
    mbedtls_mpi dpm;
    mbedtls_mpi dqm;
    mbedtls_mpi qinvm;

    mbedtls_mpi_init(&dpm);
    mbedtls_mpi_init(&dqm);
    mbedtls_mpi_init(&qinvm);

    if (!private_key || !dp_size || !dq_size || !qinv_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (!oe_private_key_is_valid(
            (oe_private_key_t*)private_key, _PRIVATE_KEY_MAGIC))
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the RSA context */
    if (!(rsa = mbedtls_pk_rsa(((oe_private_key_t*)private_key)->pk)))
        OE_RAISE(OE_FAILURE);

    if (mbedtls_rsa_export_crt(rsa, &dpm, &dqm, &qinvm) != 0)
        OE_RAISE(OE_FAILURE);

    OE_CHECK(_oe_export_mpi_param(&dpm, dp, dp_size));
    OE_CHECK(_oe_export_mpi_param(&dqm, dq, dq_size));
    OE_CHECK(_oe_export_mpi_param(&qinvm, qinv, qinv_size));
    result = OE_OK;

done:
    mbedtls_mpi_free(&dpm);
    mbedtls_mpi_free(&dqm);
    mbedtls_mpi_free(&qinvm);

    if (result != OE_OK)
    {
        if (dp)
            oe_memset_s(dp, *dp_size, 0, *dp_size);
        if (dq)
            oe_memset_s(dq, *dq_size, 0, *dq_size);
        if (qinv)
            oe_memset_s(qinv, *qinv_size, 0, *qinv_size);
    }

    return result;
}

oe_result_t oe_rsa_public_key_load_parameters(
    oe_rsa_public_key_t* public_key,
    const uint8_t* m,
    size_t m_size,
    const uint8_t* e,
    size_t e_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_public_key_t* key = (oe_public_key_t*)public_key;
    const mbedtls_pk_info_t* info = NULL;
    mbedtls_rsa_context* rsa;
    int rc = 0;

    if (!public_key)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Initialize structures */
    oe_public_key_init(key, NULL, NULL, _PUBLIC_KEY_MAGIC);

    /* Lookup the RSA info */
    if (!(info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)))
        OE_RAISE(OE_UNSUPPORTED);

    rc = mbedtls_pk_setup(&key->pk, info);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Setup RSA key. */
    rsa = mbedtls_pk_rsa(key->pk);
    mbedtls_rsa_init(rsa, MBEDTLS_RSA_PKCS_V15, 0);

    rc = mbedtls_rsa_import_raw(
        rsa, m, m_size, NULL, 0, NULL, 0, NULL, 0, e, e_size);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Sanity checks. */
    rc = mbedtls_rsa_check_pubkey(rsa);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    result = OE_OK;

done:
    if (public_key && result != OE_OK)
        oe_public_key_release(key, _PUBLIC_KEY_MAGIC);
    return result;
}

oe_result_t oe_rsa_private_key_load_parameters(
    oe_rsa_private_key_t* private_key,
    const uint8_t* p,
    size_t p_size,
    const uint8_t* q,
    size_t q_size,
    const uint8_t* e,
    size_t e_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_private_key_t* key = (oe_private_key_t*)private_key;
    const mbedtls_pk_info_t* info = NULL;
    mbedtls_rsa_context* rsa;
    int rc = 0;

    if (!private_key)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Initialize structures */
    oe_private_key_init(key, NULL, NULL, _PRIVATE_KEY_MAGIC);

    /* Lookup the RSA info */
    if (!(info = mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)))
        OE_RAISE(OE_UNSUPPORTED);

    rc = mbedtls_pk_setup(&key->pk, info);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Setup RSA key. */
    rsa = mbedtls_pk_rsa(key->pk);
    mbedtls_rsa_init(rsa, MBEDTLS_RSA_PKCS_V15, 0);

    rc = mbedtls_rsa_import_raw(
        rsa, NULL, 0, p, p_size, q, q_size, NULL, 0, e, e_size);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    /* Sanity checks. */
    rc = mbedtls_rsa_complete(rsa);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    rc = mbedtls_rsa_check_privkey(rsa);
    if (rc != 0)
        OE_RAISE_MSG(OE_FAILURE, "rc = 0x%x\n", rc);

    result = OE_OK;

done:
    if (private_key && result != OE_OK)
        oe_private_key_release(key, _PRIVATE_KEY_MAGIC);
    return result;
}
