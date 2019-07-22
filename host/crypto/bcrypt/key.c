// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "key.h"
#include <assert.h>
#include <openenclave/bits/safecrt.h>
//#include <openenclave/bits/safemath.h>
#include <openenclave/internal/pem.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include "bcrypt.h"
#include "pem.h"
//#include <string.h>
//#include "init.h"

bool oe_bcrypt_key_is_valid(const oe_bcrypt_key_t* impl, uint64_t magic)
{
    return impl && impl->magic == magic && impl->pkey;
}

oe_result_t oe_bcrypt_key_free(oe_bcrypt_key_t* key, uint64_t magic)
{
    oe_result_t result = OE_UNEXPECTED;

    if (key)
    {
        /* Check parameter */
        if (!oe_bcrypt_key_is_valid(key, magic))
            OE_RAISE(OE_INVALID_PARAMETER);

        /* Release the key */
        BCryptDestroyKey(key->pkey);

        /* Clear the fields of the implementation */
        oe_secure_zero_fill(key, sizeof(*key));
    }

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_bcrypt_key_get_blob(
    const oe_bcrypt_key_t* key,
    uint64_t magic,
    LPCWSTR blob_type,
    BYTE** blob_data,
    ULONG* blob_size)
{
    oe_result_t result = OE_UNEXPECTED;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG export_size = 0;
    ULONG output_size = 0;
    BYTE* export_data = NULL;

    if (blob_data)
        *blob_data = NULL;

    if (blob_size)
        *blob_size = 0;

    /* Check parameter */
    if (!oe_bcrypt_key_is_valid(key, magic))
        OE_RAISE(OE_INVALID_PARAMETER);

    status =
        BCryptExportKey(key->pkey, NULL, blob_type, NULL, 0, &export_size, 0);
    if (!BCRYPT_SUCCESS(status))
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR, "BCryptExportKey failed, err=%#x\n", status);

    export_data = malloc(export_size);
    if (!export_data)
        OE_RAISE(OE_OUT_OF_MEMORY);

    status = BCryptExportKey(
        key->pkey, NULL, blob_type, export_data, export_size, &output_size, 0);
    if (!BCRYPT_SUCCESS(status))
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR, "BCryptExportKey failed, err=%#x\n", status);

    if (output_size != export_size)
        OE_RAISE_MSG(
            OE_UNEXPECTED,
            "BCryptExportKey wrote:%#x bytes, expected:%#x bytes\n",
            output_size,
            export_size);

    *blob_size = export_size;
    *blob_data = export_data;
    export_data = NULL;

    result = OE_OK;

done:
    if (export_data)
        free(export_data);

    return result;
}

oe_result_t oe_bcrypt_read_key_pem(
    const uint8_t* pem_data,
    size_t pem_size,
    oe_bcrypt_key_t* key,
    oe_bcrypt_key_format_t key_args,
    uint64_t magic)
{
    oe_result_t result = OE_UNEXPECTED;
    uint8_t* der_data = NULL;
    DWORD der_size = 0;
    uint8_t* key_blob = NULL;
    DWORD key_blob_size = 0;
    BCRYPT_KEY_HANDLE handle = NULL;

    /* Zero-initialize the key */
    if (key)
        oe_secure_zero_fill(key, sizeof(oe_bcrypt_key_t));

    /* Check parameters */
    if (!pem_data || !pem_size || pem_size > OE_INT_MAX || !key ||
        !key_args.encoding || !key_args.key_algorithm ||
        !key_args.key_blob_type)
    {
        OE_RAISE(OE_INVALID_PARAMETER);
    }

    /* Must have pem_size-1 non-zero characters followed by zero-terminator */
    if (strnlen((const char*)pem_data, pem_size) != pem_size - 1)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Convert PEM to DER. */
    OE_CHECK(oe_bcrypt_pem_to_der(pem_data, pem_size, &der_data, &der_size));

    /* Check if the key is ASN encoded as an X509 public key property */
    BOOL success = CryptDecodeObjectEx(
        X509_ASN_ENCODING,
        X509_PUBLIC_KEY_INFO,
        der_data,
        der_size,
        CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
        NULL,
        &key_blob,
        &key_blob_size);

    if (success)
    {
        /* Import the X509 public key */
        PCERT_PUBLIC_KEY_INFO keyinfo = (PCERT_PUBLIC_KEY_INFO)key_blob;

        success = CryptImportPublicKeyInfoEx2(
            X509_ASN_ENCODING, keyinfo, 0, NULL, &handle);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptImportPublicKeyInfo2 failed (err=%#x)\n",
                GetLastError());
    }
    else
    {
        /* The key is not encoded with the X509 public key attributes,
         * try to decode it directly with the specified key type OID instead.
         * For example, OpenSSL-generated RSA private key-pairs.
         *
         * Note that this does not support oe_ec_private_key_write_pem
         * because of the additional complexity required. See bcrypt/ec.c.
         */
        success = CryptDecodeObjectEx(
            X509_ASN_ENCODING,
            key_args.encoding,
            der_data,
            der_size,
            CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            NULL,
            &key_blob,
            &key_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptDecodeObjectEx failed (err=%#x)\n",
                GetLastError());

        NTSTATUS status = BCryptImportKeyPair(
            key_args.key_algorithm,
            NULL,
            key_args.key_blob_type,
            &handle,
            key_blob,
            key_blob_size,
            0);

        if (!BCRYPT_SUCCESS(status))
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "BCryptImportKeyPair failed (err=%#x)\n",
                status);
    }

    /* Wrap BCrypt key handle as oe_bcrypt_key_t. */
    key->magic = magic;
    key->pkey = handle;
    handle = NULL;

    result = OE_OK;

done:
    if (handle)
        BCryptDestroyKey(handle);

    /*
     * Make sure to zero out all confidential (private key) data.
     * Note that key_blob must be freed with LocalFree and before der_data
     * due to the use of CRYPT_DECODE_NOCOPY_FLAG in CryptObjectDecodeEx.
     */
    if (key_blob)
    {
        oe_secure_zero_fill(key_blob, key_blob_size);
        LocalFree(key_blob);
        key_blob_size = 0;
    }

    if (der_data)
    {
        oe_secure_zero_fill(der_data, der_size);
        free(der_data);
        der_size = 0;
    }

    return result;
}

/* TODO: Also handle private keys as needed for EC asym key scenario */
oe_result_t oe_bcrypt_write_key_pem(
    const oe_bcrypt_key_t* key,
    oe_bcrypt_key_format_t key_args,
    uint64_t magic,
    uint8_t* pem_data,
    size_t* pem_size)
{
    oe_result_t result = OE_UNEXPECTED;
    // uint8_t* key_blob = NULL;
    // DWORD key_blob_size = 0;
    // uint8_t* asn_key_blob = NULL;
    // DWORD asn_key_blob_size = 0;
    uint8_t* keyinfo = NULL;
    DWORD keyinfo_size = 0;
    uint8_t* der_blob = NULL;
    DWORD der_blob_size = 0;
    uint8_t* pem_blob = NULL;
    size_t pem_blob_size = 0;

    /* Check parameters */
    if (!oe_bcrypt_key_is_valid(key, magic) || !key_args.encoding ||
        /*!key_args.key_algorithm ||*/ !key_args.key_blob_type || !pem_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* If buffer is null, then size must be zero */
    if (!pem_data && *pem_size != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    ///* Get the key blob from key handle */
    // OE_CHECK(oe_bcrypt_key_get_blob(
    //    key, magic, key_args.key_blob_type, &key_blob, &key_blob_size));

    ///* ASN encode the key data blob */
    // BOOL success = CryptEncodeObjectEx(
    //    X509_ASN_ENCODING,
    //    key_args.encoding,
    //    key_blob,
    //    CRYPT_ENCODE_ALLOC_FLAG,
    //    NULL,
    //    &asn_key_blob,
    //    &asn_key_blob_size);

    // if (!success)
    //    OE_RAISE_MSG(
    //        OE_CRYPTO_ERROR,
    //        "CryptEncodeObjectEx failed (err=%#x)\n",
    //        GetLastError());

    ///* Encode the key data into X509 key info format */
    // CERT_PUBLIC_KEY_INFO keyinfo = {
    //    .Algorithm = (LPSTR)key_args.encoding,
    //    .PublicKey = {.pbData = asn_key_blob, .cbData = asn_key_blob_size}};

    BOOL success = CryptExportPublicKeyInfoFromBCryptKeyHandle(
        key->pkey,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        (LPSTR)key_args.encoding,
        0,
        NULL,
        NULL,
        &keyinfo_size);

    /* Should return true and set keyinfo_size when given a null buffer */
    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptExportPublicKeyInfoFromBCryptKeyHandle failed (err=%#x)\n",
            GetLastError());

    keyinfo = (uint8_t*)malloc(keyinfo_size);
    if (keyinfo == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    success = CryptExportPublicKeyInfoFromBCryptKeyHandle(
        key->pkey,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        (LPSTR)key_args.encoding,
        0,
        NULL,
        (PCERT_PUBLIC_KEY_INFO)keyinfo,
        &keyinfo_size);

    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptExportPublicKeyInfoFromBCryptKeyHandle failed (err=%#x)\n",
            GetLastError());

    /* Encode the keyinfo structure as a X509 public key */
    success = CryptEncodeObjectEx(
        X509_ASN_ENCODING,
        X509_PUBLIC_KEY_INFO,
        keyinfo,
        CRYPT_ENCODE_ALLOC_FLAG,
        NULL,
        &der_blob,
        &der_blob_size);

    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptEncodeObjectEx failed (err=%#x)\n",
            GetLastError());

    /* Convert DER to PEM. */
    OE_CHECK(oe_bcrypt_der_to_pem(
        der_blob, der_blob_size, &pem_blob, &pem_blob_size));

    /* Check buffer size and populate required size */
    if (*pem_size < pem_blob_size)
    {
        *pem_size = pem_blob_size;
        OE_RAISE_NO_TRACE(OE_BUFFER_TOO_SMALL);
    }

    /* Copy result to output buffer */
    OE_CHECK(oe_memcpy_s(pem_data, *pem_size, pem_blob, pem_blob_size));
    *pem_size = pem_blob_size;

    result = OE_OK;

done:
    if (pem_blob)
    {
        oe_secure_zero_fill(pem_blob, pem_blob_size);
        free(pem_blob);
        pem_blob_size = 0;
    }

    if (der_blob)
    {
        oe_secure_zero_fill(der_blob, der_blob_size);
        LocalFree(der_blob);
        der_blob_size = 0;
    }

    if (keyinfo)
    {
        oe_secure_zero_fill(keyinfo, keyinfo_size);
        free(keyinfo);
        keyinfo_size = 0;
    }

    // if (asn_key_blob)
    //{
    //    oe_secure_zero_fill(asn_key_blob, asn_key_blob_size);
    //    LocalFree(asn_key_blob);
    //    asn_key_blob_size = 0;
    //}

    // if (key_blob)
    //{
    //    oe_secure_zero_fill(key_blob, key_blob_size);
    //    free(key_blob);
    //    key_blob_size = 0;
    //}

    return result;
}

void oe_bcrypt_key_init(
    oe_bcrypt_key_t* key,
    BCRYPT_KEY_HANDLE* pkey,
    uint64_t magic)
{
    key->magic = magic;
    key->pkey = pkey;
}

// void oe_private_key_init(
//    oe_private_key_t* private_key,
//    EVP_PKEY* pkey,
//    uint64_t magic)
//{
//    oe_private_key_t* impl = (oe_private_key_t*)private_key;
//    impl->magic = magic;
//    impl->pkey = pkey;
//}
//
// oe_result_t oe_private_key_read_pem(
//    const uint8_t* pem_data,
//    size_t pem_size,
//    oe_private_key_t* key,
//    int key_type,
//    uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//    oe_private_key_t* impl = (oe_private_key_t*)key;
//    BIO* bio = NULL;
//    EVP_PKEY* pkey = NULL;
//
//    /* Initialize the key output parameter */
//    if (impl)
//        oe_secure_zero_fill(impl, sizeof(oe_private_key_t));
//
//    /* Check parameters */
//    if (!pem_data || !pem_size || pem_size > OE_INT_MAX || !impl)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Must have pem_size-1 non-zero characters followed by zero-terminator */
//    if (strnlen((const char*)pem_data, pem_size) != pem_size - 1)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Initialize OpenSSL */
//    oe_initialize_openssl();
//
//    /* Create a BIO object for reading the PEM data */
//    if (!(bio = BIO_new_mem_buf(pem_data, (int)pem_size)))
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Read the key object */
//    if (!(pkey = PEM_read_bio_PrivateKey(bio, &pkey, NULL, NULL)))
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Verify that it is the right key type */
//    if (EVP_PKEY_id(pkey) != key_type)
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Initialize the key */
//    impl->magic = magic;
//    impl->pkey = pkey;
//    pkey = NULL;
//
//    result = OE_OK;
//
// done:
//
//    if (pkey)
//        EVP_PKEY_free(pkey);
//
//    if (bio)
//        BIO_free(bio);
//
//    return result;
//}
//
// oe_result_t oe_public_key_read_pem(
//    const uint8_t* pem_data,
//    size_t pem_size,
//    oe_public_key_t* key,
//    int key_type,
//    uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//    BIO* bio = NULL;
//    EVP_PKEY* pkey = NULL;
//    oe_public_key_t* impl = (oe_public_key_t*)key;
//
//    /* Zero-initialize the key */
//    if (impl)
//        oe_secure_zero_fill(impl, sizeof(oe_public_key_t));
//
//    /* Check parameters */
//    if (!pem_data || !pem_size || pem_size > OE_INT_MAX || !impl)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Must have pem_size-1 non-zero characters followed by zero-terminator */
//    if (strnlen((const char*)pem_data, pem_size) != pem_size - 1)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Initialize OpenSSL */
//    oe_initialize_openssl();
//
//    /* Create a BIO object for reading the PEM data */
//    if (!(bio = BIO_new_mem_buf(pem_data, (int)pem_size)))
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Read the key object */
//    if (!(pkey = PEM_read_bio_PUBKEY(bio, &pkey, NULL, NULL)))
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Verify that it is the right key type */
//    if (EVP_PKEY_id(pkey) != key_type)
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Initialize the key */
//    impl->magic = magic;
//    impl->pkey = pkey;
//    pkey = NULL;
//
//    result = OE_OK;
//
// done:
//
//    if (pkey)
//        EVP_PKEY_free(pkey);
//
//    if (bio)
//        BIO_free(bio);
//
//    return result;
//}
//
// oe_result_t oe_private_key_write_pem(
//    const oe_private_key_t* private_key,
//    uint8_t* data,
//    size_t* size,
//    oe_private_key_write_pem_callback private_key_write_pem_callback,
//    uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//    const oe_private_key_t* impl = (const oe_private_key_t*)private_key;
//    BIO* bio = NULL;
//    const char null_terminator = '\0';
//
//    /* Check parameters */
//    if (!oe_private_key_is_valid(impl, magic) || !size)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* If buffer is null, then size must be zero */
//    if (!data && *size != 0)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Create memory BIO object to write key to */
//    if (!(bio = BIO_new(BIO_s_mem())))
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Write key to BIO */
//    OE_CHECK(private_key_write_pem_callback(bio, impl->pkey));
//
//    /* Write a NULL terminator onto BIO */
//    if (BIO_write(bio, &null_terminator, sizeof(null_terminator)) <= 0)
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Copy the BIO onto caller's memory */
//    {
//        BUF_MEM* mem;
//
//        if (!BIO_get_mem_ptr(bio, &mem))
//            OE_RAISE(OE_CRYPTO_ERROR);
//
//        /* If buffer is too small */
//        if (*size < mem->length)
//        {
//            *size = mem->length;
//            OE_RAISE(OE_BUFFER_TOO_SMALL);
//        }
//
//        /* Copy result to output buffer */
//        OE_CHECK(oe_memcpy_s(data, *size, mem->data, mem->length));
//        *size = mem->length;
//    }
//
//    result = OE_OK;
//
// done:
//
//    if (bio)
//        BIO_free(bio);
//
//    return result;
//}
//
// oe_result_t oe_public_key_write_pem(
//    const oe_public_key_t* key,
//    uint8_t* data,
//    size_t* size,
//    uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//   BIO* bio = NULL;
//   const oe_public_key_t* impl = (const oe_public_key_t*)key;
//   const char null_terminator = '\0';

//   /* Check parameters */
//   if (!oe_public_key_is_valid(impl, magic) || !size)
//       OE_RAISE(OE_INVALID_PARAMETER);

//   /* If buffer is null, then size must be zero */
//   if (!data && *size != 0)
//       OE_RAISE(OE_INVALID_PARAMETER);

//   /* Create memory BIO object to write key to */
//   if (!(bio = BIO_new(BIO_s_mem())))
//       OE_RAISE(OE_CRYPTO_ERROR);

//   /* Write key to BIO */
//   if (!PEM_write_bio_PUBKEY(bio, impl->pkey))
//       OE_RAISE(OE_CRYPTO_ERROR);

//   /* Write a NULL terminator onto BIO */
//   if (BIO_write(bio, &null_terminator, sizeof(null_terminator)) <= 0)
//       OE_RAISE(OE_CRYPTO_ERROR);

//   /* Copy the BIO onto caller's memory */
//   {
//       BUF_MEM* mem;

//       if (!BIO_get_mem_ptr(bio, &mem))
//           OE_RAISE(OE_CRYPTO_ERROR);

//       /* If buffer is too small */
//       if (*size < mem->length)
//       {
//           *size = mem->length;
//           OE_RAISE(OE_BUFFER_TOO_SMALL);
//       }

//       /* Copy result to output buffer */
//       OE_CHECK(oe_memcpy_s(data, *size, mem->data, mem->length));
//       *size = mem->length;
//   }

//   result = OE_OK;

// done:

//   if (bio)
//       BIO_free(bio);

//   return result;
//}

// oe_result_t oe_private_key_free(oe_private_key_t* key, uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//
//    if (key)
//    {
//        oe_private_key_t* impl = (oe_private_key_t*)key;
//
//        /* Check parameter */
//        if (!oe_private_key_is_valid(impl, magic))
//            OE_RAISE(OE_INVALID_PARAMETER);
//
//        /* Release the key */
//        EVP_PKEY_free(impl->pkey);
//
//        /* Clear the fields of the implementation */
//        if (impl)
//            oe_secure_zero_fill(impl, sizeof(oe_private_key_t));
//    }
//
//    result = OE_OK;
//
// done:
//    return result;
//}
//
// oe_result_t oe_public_key_free(oe_public_key_t* key, uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//
//    if (key)
//    {
//        oe_public_key_t* impl = (oe_public_key_t*)key;
//
//        /* Check parameter */
//        if (!oe_public_key_is_valid(impl, magic))
//            OE_RAISE(OE_INVALID_PARAMETER);
//
//        /* Release the key */
//        EVP_PKEY_free(impl->pkey);
//
//        /* Clear the fields of the implementation */
//        if (impl)
//            oe_secure_zero_fill(impl, sizeof(oe_public_key_t));
//    }
//
//    result = OE_OK;
//
// done:
//    return result;
//}

oe_result_t oe_private_key_sign(
    const oe_private_key_t* private_key,
    const oe_bcrypt_padding_info_t* padding_info,
    const void* hash_data,
    size_t hash_size,
    uint8_t* signature,
    size_t* signature_size,
    uint64_t magic)
{
    oe_result_t result = OE_UNEXPECTED;
    const oe_bcrypt_key_t* impl = (const oe_bcrypt_key_t*)private_key;
    const oe_bcrypt_padding_info_t default_padding_info = {0};
    ULONG sig_size = 0;
    NTSTATUS status;

    if (!padding_info)
        padding_info = &default_padding_info;

    /* Check for null parameters and invalid sizes. */
    if (!oe_bcrypt_key_is_valid(impl, magic) || !hash_data || !hash_size ||
        hash_size > MAXDWORD || !signature_size || *signature_size > MAXDWORD)
    {
        OE_RAISE(OE_INVALID_PARAMETER);
    }

    /* If signature buffer is null, then signature size must be zero */
    if (!signature && *signature_size != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Determine the size of the signature; fail if buffer is too small */
    status = BCryptSignHash(
        impl->pkey,
        padding_info->config,
        (PUCHAR)hash_data,
        (ULONG)hash_size,
        NULL,
        0,
        &sig_size,
        padding_info->type);

    if (!BCRYPT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR, "BCryptSignHash failed (err=%#x)\n", status);

    if (sig_size > *signature_size)
    {
        *signature_size = sig_size;
        OE_RAISE(OE_BUFFER_TOO_SMALL);
    }

    status = BCryptSignHash(
        impl->pkey,
        padding_info->config,
        (PUCHAR)hash_data,
        (ULONG)hash_size,
        signature,
        sig_size,
        &sig_size,
        padding_info->type);

    if (!BCRYPT_SUCCESS(status))
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR, "BCryptSignHash failed (err=%#x)\n", status);

    *signature_size = sig_size;

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_public_key_verify(
    const oe_public_key_t* public_key,
    const oe_bcrypt_padding_info_t* padding_info,
    const void* hash_data,
    size_t hash_size,
    const uint8_t* signature,
    size_t signature_size,
    uint64_t magic)
{
    oe_result_t result = OE_UNEXPECTED;
    const oe_bcrypt_key_t* impl = (const oe_bcrypt_key_t*)public_key;
    const oe_bcrypt_padding_info_t default_padding_info = {0};
    NTSTATUS status;

    /* Check parameters */
    if (!oe_bcrypt_key_is_valid(impl, magic) || !hash_data || !hash_size ||
        hash_size > MAXDWORD || !signature || !signature_size ||
        signature_size > MAXDWORD)
    {
        OE_RAISE(OE_INVALID_PARAMETER);
    }

    if (!padding_info)
        padding_info = &default_padding_info;

    status = BCryptVerifySignature(
        impl->pkey,
        padding_info->config,
        (PUCHAR)hash_data,
        (ULONG)hash_size,
        (PUCHAR)signature,
        (ULONG)signature_size,
        padding_info->type);

    if (!BCRYPT_SUCCESS(status))
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "BCryptVerifySignature failed (err=%#x)\n",
            status);

    result = OE_OK;

done:
    return result;
}
