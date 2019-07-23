// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../rsa.h"
#include "bcrypt.h"
#include "key.h"
#include "rsa.h"

#include <openenclave/bits/safecrt.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/rsa.h>
#include <openenclave/internal/utils.h>

/*
 * Magic numbers for the RSA key implementation structures. These values
 * were copied from the linux rsa.c file. These can be consolidated once
 * more crypto code is ported to Windows.
 */
static const uint64_t _PRIVATE_KEY_MAGIC = 0x7bf635929a714b2c;
static const uint64_t _PUBLIC_KEY_MAGIC = 0x8f8f72170025426d;

OE_STATIC_ASSERT(sizeof(oe_public_key_t) <= sizeof(oe_rsa_public_key_t));
OE_STATIC_ASSERT(sizeof(oe_private_key_t) <= sizeof(oe_rsa_private_key_t));

static const oe_bcrypt_key_format_t _PRIVATE_RSA_KEY_ARGS = {
    CNG_RSA_PRIVATE_KEY_BLOB,
    BCRYPT_RSA_ALG_HANDLE,
    BCRYPT_RSAPRIVATE_BLOB};

static const oe_bcrypt_key_format_t _PUBLIC_RSA_KEY_ARGS = {
    szOID_RSA_RSA,
    BCRYPT_RSA_ALG_HANDLE,
    BCRYPT_RSAPUBLIC_BLOB};

/* Caller is responsible for freeing padding_info->config */
static oe_result_t _get_padding_info(
    oe_hash_type_t hash_type,
    size_t hash_size,
    oe_bcrypt_padding_info_t* padding_info)
{
    oe_result_t result = OE_UNEXPECTED;
    PCWSTR hash_algorithm = NULL;

    /* Check for support hash types and correct sizes. */
    switch (hash_type)
    {
        case OE_HASH_TYPE_SHA256:
            if (hash_size != 32)
                OE_RAISE(OE_INVALID_PARAMETER);
            hash_algorithm = BCRYPT_SHA256_ALGORITHM;
            break;
        case OE_HASH_TYPE_SHA512:
            if (hash_size != 64)
                OE_RAISE(OE_INVALID_PARAMETER);
            hash_algorithm = BCRYPT_SHA512_ALGORITHM;
            break;
        default:
            OE_RAISE(OE_INVALID_PARAMETER);
    }

    /*
     * Note that we use the less secure PKCS1 signature padding
     * because Intel requires it for SGX enclave signatures.
     */
    padding_info->type = BCRYPT_PAD_PKCS1;
    padding_info->config = malloc(sizeof(BCRYPT_PKCS1_PADDING_INFO));
    BCRYPT_PKCS1_PADDING_INFO* info =
        (BCRYPT_PKCS1_PADDING_INFO*)(padding_info->config);
    info->pszAlgId = hash_algorithm;

    result = OE_OK;

done:
    return result;
}

static oe_result_t _get_public_rsa_blob_info(
    BCRYPT_KEY_HANDLE key,
    uint8_t** buffer,
    ULONG* buffer_size)
{
    oe_result_t result = OE_UNEXPECTED;
    NTSTATUS status;
    uint8_t* buffer_local = NULL;
    ULONG buffer_local_size = 0;
    BCRYPT_RSAKEY_BLOB* key_header;

    if (!key || !buffer || !buffer_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    status = BCryptExportKey(
        key, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &buffer_local_size, 0);

    if (!BCRYPT_SUCCESS(status))
        OE_RAISE(OE_CRYPTO_ERROR);

    buffer_local = (uint8_t*)malloc(buffer_local_size);
    if (buffer_local == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    status = BCryptExportKey(
        key,
        NULL,
        BCRYPT_RSAPUBLIC_BLOB,
        buffer_local,
        buffer_local_size,
        &buffer_local_size,
        0);

    if (!BCRYPT_SUCCESS(status))
        OE_RAISE(OE_CRYPTO_ERROR);

    /* Sanity check to ensure we get modulus and public exponent. */
    key_header = (BCRYPT_RSAKEY_BLOB*)buffer_local;
    if (key_header->cbPublicExp == 0 || key_header->cbModulus == 0)
        OE_RAISE(OE_FAILURE);

    *buffer = buffer_local;
    *buffer_size = buffer_local_size;
    buffer_local = NULL;
    result = OE_OK;

done:
    if (buffer_local)
    {
        oe_secure_zero_fill(buffer_local, buffer_local_size);
        free(buffer_local);
        buffer_local_size = 0;
    }

    return result;
}

void oe_rsa_public_key_init(
    oe_rsa_public_key_t* public_key,
    BCRYPT_KEY_HANDLE* pkey)
{
    oe_bcrypt_key_init((oe_bcrypt_key_t*)public_key, pkey, _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_private_key_read_pem(
    oe_rsa_private_key_t* private_key,
    const uint8_t* pem_data,
    size_t pem_size)
{
    return oe_bcrypt_read_key_pem(
        pem_data,
        pem_size,
        (oe_bcrypt_key_t*)private_key,
        _PRIVATE_RSA_KEY_ARGS,
        _PRIVATE_KEY_MAGIC);
}

/* Used by tests/crypto/rse_tests
 * Removing the test since there's no production use for it.
 */
// oe_result_t oe_rsa_private_key_write_pem(
//    const oe_rsa_private_key_t* private_key,
//    uint8_t* pem_data,
//    size_t* pem_size)
//{
//    return oe_private_key_write_pem(
//        (const oe_private_key_t*)private_key,
//        pem_data,
//        pem_size,
//        _private_key_write_pem_callback,
//        _PRIVATE_KEY_MAGIC);
//}

/* Used by tests/crypto/rsa_tests */
oe_result_t oe_rsa_public_key_read_pem(
    oe_rsa_public_key_t* public_key,
    const uint8_t* pem_data,
    size_t pem_size)
{
    return oe_bcrypt_read_key_pem(
        pem_data,
        pem_size,
        (oe_bcrypt_key_t*)public_key,
        _PUBLIC_RSA_KEY_ARGS,
        _PUBLIC_KEY_MAGIC);
}

/* Used by tests/crypto/rsa_tests
 * Also used by common/cert.c for tlsverifier.c now */
oe_result_t oe_rsa_public_key_write_pem(
    const oe_rsa_public_key_t* public_key,
    uint8_t* pem_data,
    size_t* pem_size)
{
    return oe_bcrypt_write_key_pem(
        (const oe_bcrypt_key_t*)public_key,
        _PUBLIC_RSA_KEY_ARGS,
        _PUBLIC_KEY_MAGIC,
        pem_data,
        pem_size);
}

oe_result_t oe_rsa_private_key_free(oe_rsa_private_key_t* private_key)
{
    return oe_bcrypt_key_free(
        (oe_bcrypt_key_t*)private_key, _PRIVATE_KEY_MAGIC);
}

oe_result_t oe_rsa_public_key_free(oe_rsa_public_key_t* public_key)
{
    return oe_bcrypt_key_free((oe_bcrypt_key_t*)public_key, _PUBLIC_KEY_MAGIC);
}

oe_result_t oe_rsa_private_key_sign(
    const oe_rsa_private_key_t* private_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    uint8_t* signature,
    size_t* signature_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_bcrypt_padding_info_t padding_info = {0};
    OE_CHECK(_get_padding_info(hash_type, hash_size, &padding_info));
    OE_CHECK(oe_private_key_sign(
        (oe_private_key_t*)private_key,
        &padding_info,
        hash_data,
        hash_size,
        signature,
        signature_size,
        _PRIVATE_KEY_MAGIC));

    result = OE_OK;

done:
    if (padding_info.config)
        free(padding_info.config);

    return result;
}

/* Used by tests/crypto/rsa_tests */
oe_result_t oe_rsa_public_key_verify(
    const oe_rsa_public_key_t* public_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    const uint8_t* signature,
    size_t signature_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_bcrypt_padding_info_t padding_info = {0};
    OE_CHECK(_get_padding_info(hash_type, hash_size, &padding_info));
    OE_CHECK(oe_public_key_verify(
        (oe_public_key_t*)public_key,
        &padding_info,
        hash_data,
        hash_size,
        signature,
        signature_size,
        _PUBLIC_KEY_MAGIC));

    result = OE_OK;

done:
    if (padding_info.config)
        free(padding_info.config);

    return result;
}

/* Used by tests/crypto/rsa_tests */
// ATTN: BCrypt does not support arbitrary modulus values in key
// generation, cannot be supported
oe_result_t oe_rsa_generate_key_pair(
    uint64_t bits,
    uint64_t exponent,
    oe_rsa_private_key_t* private_key,
    oe_rsa_public_key_t* public_key)
{
    return OE_UNSUPPORTED;
    //    oe_result_t result = OE_UNEXPECTED;
    //    BCRYPT_ALG_HANDLE rsa_handle = NULL;
    //    BCRYPT_KEY_HANDLE key = NULL;
    //    NTSTATUS status =
    //        BCryptOpenAlgorithmProvider(&rsa_handle, BCRYPT_RSA_ALGORITHM,
    //        NULL, 0);
    //    if (!BCRYPT_SUCCESS(status))
    //        OE_RAISE_MSG(
    //            OE_CRYPTO_ERROR,
    //            "BCryptOpenAlgorithmProvider failed, err=%#x",
    //            status);
    //
    //    status = BCryptGenerateKeyPair(rsa_handle, &key, bits, 0);
    //
    // done:
    //    return result;
}

oe_result_t oe_rsa_public_key_get_modulus(
    const oe_rsa_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size)
{
    oe_result_t result = OE_UNEXPECTED;
    const oe_public_key_t* impl = (const oe_public_key_t*)public_key;
    uint8_t* keybuf = NULL;
    ULONG keybuf_size;
    BCRYPT_RSAKEY_BLOB* keyblob;

    /* Check for null parameters and invalid sizes. */
    if (!oe_bcrypt_key_is_valid(impl, _PUBLIC_KEY_MAGIC) || !buffer_size ||
        *buffer_size > MAXDWORD)
    {
        OE_RAISE(OE_INVALID_PARAMETER);
    }

    /* If buffer is null, then buffer_size must be zero */
    if (!buffer && *buffer_size != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    OE_CHECK(_get_public_rsa_blob_info(impl->pkey, &keybuf, &keybuf_size));

    keyblob = (BCRYPT_RSAKEY_BLOB*)keybuf;
    if (keyblob->cbModulus > *buffer_size)
    {
        *buffer_size = keyblob->cbModulus;
        OE_RAISE(OE_BUFFER_TOO_SMALL);
    }

    /*
     * A RSA public key BCrypt blob has the following format in contiguous
     * memory:
     *   BCRYPT_RSAKEY_BLOB struct
     *   PublicExponent[cbPublicExp] in big endian
     *   Modulus[cbModulus] in big endian
     */
    OE_CHECK(oe_memcpy_s(
        buffer,
        *buffer_size,
        keybuf + sizeof(*keyblob) + keyblob->cbPublicExp,
        keyblob->cbModulus));

    *buffer_size = keyblob->cbModulus;
    result = OE_OK;

done:
    if (keybuf)
    {
        oe_secure_zero_fill(keybuf, keybuf_size);
        free(keybuf);
        keybuf_size = 0;
    }

    return result;
}

oe_result_t oe_rsa_public_key_get_exponent(
    const oe_rsa_public_key_t* public_key,
    uint8_t* buffer,
    size_t* buffer_size)
{
    oe_result_t result = OE_UNEXPECTED;
    const oe_public_key_t* impl = (const oe_public_key_t*)public_key;
    uint8_t* keybuf = NULL;
    ULONG keybuf_size;
    BCRYPT_RSAKEY_BLOB* keyblob;

    /* Check for null parameters and invalid sizes. */
    if (!oe_bcrypt_key_is_valid(impl, _PUBLIC_KEY_MAGIC) || !buffer_size ||
        *buffer_size > MAXDWORD)
    {
        OE_RAISE(OE_INVALID_PARAMETER);
    }

    /* If buffer is null, then buffer_size must be zero */
    if (!buffer && *buffer_size != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    OE_CHECK(_get_public_rsa_blob_info(impl->pkey, &keybuf, &keybuf_size));

    keyblob = (BCRYPT_RSAKEY_BLOB*)keybuf;
    if (keyblob->cbPublicExp > *buffer_size)
    {
        *buffer_size = keyblob->cbPublicExp;
        OE_RAISE(OE_BUFFER_TOO_SMALL);
    }

    /*
     * A RSA public key BCrypt blob has the following format in contiguous
     * memory:
     *   BCRYPT_RSAKEY_BLOB struct
     *   PublicExponent[cbPublicExp] in big endian
     *   Modulus[cbModulus] in big endian
     */
    OE_CHECK(oe_memcpy_s(
        buffer, *buffer_size, keybuf + sizeof(*keyblob), keyblob->cbPublicExp));

    *buffer_size = keyblob->cbPublicExp;
    result = OE_OK;

done:
    if (keybuf)
    {
        oe_secure_zero_fill(keybuf, keybuf_size);
        free(keybuf);
        keybuf_size = 0;
    }

    return result;
}

/* Used by tests/crypto/rsa_tests */
oe_result_t oe_rsa_public_key_equal(
    const oe_rsa_public_key_t* public_key1,
    const oe_rsa_public_key_t* public_key2,
    bool* equal)
{
    oe_result_t result = OE_UNEXPECTED;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    /* key1 and key2 are both BCRYPT_RSAKEY_BLOB structures
     * which should be comparable as raw byte buffers.
     */
    BYTE* key1 = NULL;
    BYTE* key2 = NULL;
    ULONG key1_size = 0;
    ULONG key2_size = 0;

    if (equal)
        *equal = false;
    else
        OE_RAISE(OE_INVALID_PARAMETER);

    OE_CHECK(oe_bcrypt_key_get_blob(
        (oe_bcrypt_key_t*)public_key1,
        _PUBLIC_KEY_MAGIC,
        BCRYPT_RSAPUBLIC_BLOB,
        &key1,
        &key1_size));

    OE_CHECK(oe_bcrypt_key_get_blob(
        (oe_bcrypt_key_t*)public_key2,
        _PUBLIC_KEY_MAGIC,
        BCRYPT_RSAPUBLIC_BLOB,
        &key2,
        &key2_size));

    if ((key1_size == key2_size) &&
        oe_constant_time_mem_equal(key1, key2, key1_size))
    {
        *equal = true;
    }

    result = OE_OK;

done:
    if (key1)
    {
        oe_secure_zero_fill(key1, key1_size);
        free(key1);
    }

    if (key2)
    {
        oe_secure_zero_fill(key2, key2_size);
        free(key2);
    }
    return result;
}

oe_result_t oe_rsa_get_public_key_from_private(
    const oe_rsa_private_key_t* private_key,
    oe_rsa_public_key_t* public_key)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_private_key_t* impl = (oe_private_key_t*)private_key;
    uint8_t* keybuf = NULL;
    ULONG keybuf_size;

    if (!oe_bcrypt_key_is_valid(impl, _PRIVATE_KEY_MAGIC) || !public_key)
        OE_RAISE(OE_INVALID_PARAMETER);

    OE_CHECK(_get_public_rsa_blob_info(impl->pkey, &keybuf, &keybuf_size));

    /*
     * Export the key blob to a public key. Note that the private key blob has
     * the modulus and the exponent already, so we can just use it to import
     * the public key.
     */
    {
        NTSTATUS status;
        BCRYPT_KEY_HANDLE public_key_handle;

        status = BCryptImportKeyPair(
            BCRYPT_RSA_ALG_HANDLE,
            NULL,
            BCRYPT_RSAPUBLIC_BLOB,
            &public_key_handle,
            keybuf,
            keybuf_size,
            0);

        if (!BCRYPT_SUCCESS(status))
            OE_RAISE(OE_CRYPTO_ERROR);

        oe_rsa_public_key_init(public_key, public_key_handle);
    }

    result = OE_OK;

done:
    if (keybuf)
    {
        oe_secure_zero_fill(keybuf, keybuf_size);
        free(keybuf);
        keybuf_size = 0;
    }

    return result;
}
