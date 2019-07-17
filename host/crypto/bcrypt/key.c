// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "key.h"
#include <assert.h>
#include <openenclave/bits/safecrt.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/internal/pem.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
//#include <string.h>
//#include "init.h"
#include <bcrypt.h>

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

static oe_result_t _bcrypt_pem_to_der(
    const uint8_t* pem_data,
    size_t pem_size,
    uint8_t** der_data,
    DWORD* der_size)
{
    oe_result_t result = OE_UNEXPECTED;
    uint8_t* der_local = NULL;
    DWORD der_local_size = 0;
    BOOL success;

    if (!pem_data || pem_size == 0 || pem_size > MAXDWORD || !der_data ||
        !der_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Subtract 1, since BCrypt doesn't count the null terminator.*/
    pem_size--;

    success = CryptStringToBinaryA(
        (const char*)pem_data,
        (DWORD)pem_size,
        CRYPT_STRING_BASE64HEADER,
        NULL,
        &der_local_size,
        NULL,
        NULL);

    /* Should return true and set der_local_size when given a null buffer */
    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptStringToBinaryA failed (err=%#x)\n",
            GetLastError());

    der_local = (uint8_t*)malloc(der_local_size);
    if (der_local == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    success = CryptStringToBinaryA(
        (const char*)pem_data,
        (DWORD)pem_size,
        CRYPT_STRING_BASE64HEADER,
        der_local,
        &der_local_size,
        NULL,
        NULL);

    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptStringToBinaryA failed (err=%#x)\n",
            GetLastError());

    *der_data = der_local;
    *der_size = der_local_size;
    result = OE_OK;
    der_local = NULL;

done:
    if (der_local)
    {
        oe_secure_zero_fill(der_local, der_local_size);
        free(der_local);
        der_local_size = 0;
    }

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
    uint8_t* rsa_blob = NULL;
    DWORD rsa_blob_size = 0;
    BCRYPT_KEY_HANDLE handle = NULL;

    uint8_t* x509_key_blob = NULL;
    DWORD x509_key_blob_size = 0;

    if (!key || !pem_data)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Step 1: Convert PEM to DER. */
    OE_CHECK(_bcrypt_pem_to_der(pem_data, pem_size, &der_data, &der_size));

    /* Step 2: Decode DER to Crypt object. */
    // TODO: split this into separate codepaths as needed
    if (key_args.input_type == X509_PUBLIC_KEY_INFO)
    {
        BOOL success = CryptDecodeObjectEx(
            X509_ASN_ENCODING,
            X509_PUBLIC_KEY_INFO,
            der_data,
            der_size,
            CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            NULL,
            &x509_key_blob,
            &x509_key_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptDecodeObjectEx failed (err=%#x)\n",
                GetLastError());

        PCERT_PUBLIC_KEY_INFO keyinfo = (PCERT_PUBLIC_KEY_INFO)x509_key_blob;

        success = CryptDecodeObjectEx(
            X509_ASN_ENCODING,
            CNG_RSA_PUBLIC_KEY_BLOB,
            keyinfo->PublicKey.pbData,
            keyinfo->PublicKey.cbData,
            CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            NULL,
            &rsa_blob,
            &rsa_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptDecodeObjectEx failed (err=%#x)\n",
                GetLastError());
    }
    else
    {
        BOOL success = CryptDecodeObjectEx(
            X509_ASN_ENCODING,
            key_args.input_type,
            der_data,
            der_size,
            CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
            NULL,
            &rsa_blob,
            &rsa_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptDecodeObjectEx failed (err=%#x)\n",
                GetLastError());
    }

    /* Step 3: Convert the Crypt object to a BCrypt Key. */
    {
        NTSTATUS status = BCryptImportKeyPair(
            key_args.key_algorithm,
            NULL,
            key_args.key_blob_type,
            &handle,
            rsa_blob,
            rsa_blob_size,
            0);

        if (!BCRYPT_SUCCESS(status))
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "BCryptImportKeyPair failed (err=%#x)\n",
                status);
    }

    /* Step 4: Convery BCrypt Handle to OE type. */
    {
        key->magic = magic;
        key->pkey = handle;
        handle = NULL;
    }

    result = OE_OK;

done:
    if (handle)
        BCryptDestroyKey(handle);

    /*
     * Make sure to zero out all confidential (private key) data.
     * Note that rsa_blob must be freed with LocalFree and before der_data
     * due to constraints in CryptObjectDecodeEx.
     */
    if (rsa_blob)
    {
        oe_secure_zero_fill(rsa_blob, rsa_blob_size);
        LocalFree(rsa_blob);
        rsa_blob_size = 0;
    }

    if (x509_key_blob)
    {
        oe_secure_zero_fill(x509_key_blob, x509_key_blob_size);
        LocalFree(x509_key_blob);
        x509_key_blob_size = 0;
    }

    if (der_data)
    {
        oe_secure_zero_fill(der_data, der_size);
        free(der_data);
        der_size = 0;
    }

    return result;
}

static oe_result_t _bcrypt_der_to_pem(
    const BYTE* der_data,
    DWORD der_size,
    uint8_t** pem_data,
    size_t* pem_size)
{
    oe_result_t result = OE_UNEXPECTED;
    uint8_t* pem_local = NULL;
    DWORD pem_local_size = 0;
    BOOL success;

    if (!der_data || der_size == 0 || der_size > MAXDWORD || !pem_data ||
        !pem_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    success = CryptBinaryToStringA(
        der_data,
        der_size,
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
        NULL,
        &pem_local_size);

    /* Should return true and set pem_local_size when given a null buffer.
     * Resulting pem_local_size includes null terminator. */
    if (!success)
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "CryptBinaryToStringA failed (err=%#x)\n",
            GetLastError());

    /* Need to allocate and write the PEM header/footer manually because
     * BCrypt only supports the cert/CRL/CSR headers.
     * The size also accounts for LF characters at the end of each line. */
    const DWORD pem_headers_size =
        OE_PEM_BEGIN_PUBLIC_KEY_LEN + OE_PEM_END_PUBLIC_KEY_LEN + 2;
    OE_CHECK(
        oe_safe_add_u32(pem_local_size, pem_headers_size, &pem_local_size));

    pem_local = (uint8_t*)malloc(pem_local_size);
    if (pem_local == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    {
        /* Write the begin public key header */
        uint8_t* pos = pem_local;
        DWORD size_left = pem_local_size;

        OE_CHECK(oe_memcpy_s(
            pos,
            size_left,
            OE_PEM_BEGIN_PUBLIC_KEY,
            OE_PEM_BEGIN_PUBLIC_KEY_LEN));
        pos += OE_PEM_BEGIN_PUBLIC_KEY_LEN;
        size_left -= OE_PEM_BEGIN_PUBLIC_KEY_LEN;

        /* Write a LF character */
        *pos++ = 0x0A;
        size_left--;

        /* Write the encoded key data */
        DWORD written_size = size_left;
        success = CryptBinaryToStringA(
            der_data,
            der_size,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCR,
            pos,
            &written_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptBinaryToStringA failed (err=%#x)\n",
                GetLastError());

        /* Assert null-terminator at end of string.
         * This method overwrites it to extend the string for the end footer */
        pos += written_size;
        assert(*pos == '\0');
        size_left -= written_size;

        /* Write the end public key footer */
        OE_CHECK(oe_memcpy_s(
            pos, size_left, OE_PEM_END_PUBLIC_KEY, OE_PEM_END_PUBLIC_KEY_LEN));
        pos += OE_PEM_END_PUBLIC_KEY_LEN;
        size_left -= OE_PEM_END_PUBLIC_KEY_LEN;

        /* Add LF and null-terminator */
        OE_CHECK(size_left == 2 ? OE_OK : OE_UNEXPECTED);
        *pos++ = 0x0A;
        *pos++ = 0x00;
    }

    *pem_data = (uint8_t*)pem_local;
    *pem_size = (size_t)pem_local_size;
    result = OE_OK;
    pem_local = NULL;

done:
    if (pem_local)
    {
        oe_secure_zero_fill(pem_local, pem_local_size);
        free(pem_local);
        pem_local_size = 0;
    }

    return result;
}

/* TODO: generalize oe_bcrypt_key_format_t */
oe_result_t oe_bcrypt_write_key_pem(
    const oe_bcrypt_key_t* key,
    oe_bcrypt_key_format_t key_args,
    uint64_t magic,
    uint8_t* pem_data,
    size_t* pem_size)
{
    oe_result_t result = OE_UNEXPECTED;
    uint8_t* key_blob = NULL;
    DWORD key_blob_size = 0;
    uint8_t* rsa_blob = NULL;
    DWORD rsa_blob_size = 0;
    uint8_t* encoded_blob = NULL;
    DWORD encoded_blob_size = 0;
    uint8_t* pem_blob = NULL;
    size_t pem_blob_size = 0;

    /* Check parameters */
    if (!oe_bcrypt_key_is_valid(key, magic) || !key_args.input_type ||
        !key_args.key_algorithm || !key_args.key_blob_type || !pem_size)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* If buffer is null, then size must be zero */
    if (!pem_data && *pem_size != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Get the key blob from key handle */
    OE_CHECK(oe_bcrypt_key_get_blob(
        key, magic, key_args.key_blob_type, &key_blob, &key_blob_size));

    /* Step 2: Encode DER blob to ASN object. */
    // TODO: split this into separate codepaths as needed
    if (key_args.input_type == X509_PUBLIC_KEY_INFO)
    {
        BOOL success = CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            CNG_RSA_PUBLIC_KEY_BLOB,
            key_blob,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,
            &rsa_blob,
            &rsa_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptEncodeObjectEx failed (err=%#x)\n",
                GetLastError());

        // TODO: Generalize this
        CERT_PUBLIC_KEY_INFO keyinfo = {
            .Algorithm = szOID_RSA_RSA,
            .PublicKey = {.pbData = rsa_blob, .cbData = rsa_blob_size}};

        success = CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            X509_PUBLIC_KEY_INFO,
            &keyinfo,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,
            &encoded_blob,
            &encoded_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptEncodeObjectEx failed (err=%#x)\n",
                GetLastError());
    }
    else
    {
        BOOL success = CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            key_args.input_type,
            key_blob,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,
            &encoded_blob,
            &encoded_blob_size);

        if (!success)
            OE_RAISE_MSG(
                OE_CRYPTO_ERROR,
                "CryptEncodeObjectEx failed (err=%#x)\n",
                GetLastError());
    }

    /* Convert DER to PEM. */
    OE_CHECK(_bcrypt_der_to_pem(
        encoded_blob, encoded_blob_size, &pem_blob, &pem_blob_size));

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
    /*
     * Make sure to zero out all confidential (private key) data.
     * Note that encoded_blob must be freed with LocalFree and before rsa_blob
     * due to constraints in CryptObjectEncodeEx.
     */
    // if (encoded_blob)
    //{
    //    oe_secure_zero_fill(encoded_blob, encoded_blob_size);
    //    LocalFree(encoded_blob);
    //    encoded_blob_size = 0;
    //}

    // if (rsa_blob)
    //{
    //    oe_secure_zero_fill(rsa_blob, rsa_blob_size);
    //    LocalFree(rsa_blob);
    //    rsa_blob_size = 0;
    //}

    // if (pem_blob)
    //{
    //    oe_secure_zero_fill(pem_blob, pem_blob_size);
    //    free(pem_blob);
    //    pem_blob_size = 0;
    //}

    // if (key_blob)
    //{
    //    oe_secure_zero_fill(key_blob, key_blob_size);
    //    free(key_blob);
    //    key_blob_size = 0;
    //}

    return result;
}

void oe_public_key_init(
    oe_public_key_t* public_key,
    BCRYPT_KEY_HANDLE* pkey,
    uint64_t magic)
{
    oe_public_key_t* impl = (oe_public_key_t*)public_key;
    impl->magic = magic;
    impl->pkey = pkey;
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
//
// oe_result_t oe_private_key_sign(
//    const oe_private_key_t* private_key,
//    oe_hash_type_t hash_type,
//    const void* hash_data,
//    size_t hash_size,
//    uint8_t* signature,
//    size_t* signature_size,
//    uint64_t magic)
//{
//    oe_result_t result = OE_UNEXPECTED;
//    const oe_private_key_t* impl = (const oe_private_key_t*)private_key;
//    EVP_PKEY_CTX* ctx = NULL;
//
//    /* Check for null parameters */
//    if (!oe_private_key_is_valid(impl, magic) || !hash_data || !hash_size ||
//        !signature_size)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Check that hash buffer is big enough (hash_type is size of that hash)
//    */ if (hash_type > hash_size)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* If signature buffer is null, then signature size must be zero */
//    if (!signature && *signature_size != 0)
//        OE_RAISE(OE_INVALID_PARAMETER);
//
//    /* Initialize OpenSSL */
//    oe_initialize_openssl();
//
//    /* Create signing context */
//    if (!(ctx = EVP_PKEY_CTX_new(impl->pkey, NULL)))
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Initialize the signing context */
//    if (EVP_PKEY_sign_init(ctx) <= 0)
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Set the MD type for the signing operation */
//    if (EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) <= 0)
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    /* Determine the size of the signature; fail if buffer is too small */
//    {
//        size_t size;
//
//        if (EVP_PKEY_sign(ctx, NULL, &size, hash_data, hash_size) <= 0)
//            OE_RAISE(OE_CRYPTO_ERROR);
//
//        if (size > *signature_size)
//        {
//            *signature_size = size;
//            OE_RAISE(OE_BUFFER_TOO_SMALL);
//        }
//
//        *signature_size = size;
//    }
//
//    /* Compute the signature */
//    if (EVP_PKEY_sign(ctx, signature, signature_size, hash_data, hash_size) <=
//        0)
//        OE_RAISE(OE_CRYPTO_ERROR);
//
//    result = OE_OK;
//
// done:
//
//    if (ctx)
//        EVP_PKEY_CTX_free(ctx);
//
//    return result;
//}

oe_result_t oe_public_key_verify(
    const oe_public_key_t* public_key,
    oe_hash_type_t hash_type,
    const void* hash_data,
    size_t hash_size,
    const uint8_t* signature,
    size_t signature_size,
    uint64_t magic)
{
    oe_result_t result = OE_UNEXPECTED;
    const oe_bcrypt_key_t* impl = (const oe_bcrypt_key_t*)public_key;
    BCRYPT_PKCS1_PADDING_INFO pad_info = {0};

    /* Check for null parameters */
    if (!oe_bcrypt_key_is_valid(impl, magic) || !hash_data || !hash_size ||
        hash_size > MAXDWORD || !signature || !signature_size ||
        signature_size > MAXDWORD)
    {
        OE_RAISE(OE_INVALID_PARAMETER);
    }

    switch (hash_type)
    {
        case OE_HASH_TYPE_SHA256:
            pad_info.pszAlgId = BCRYPT_SHA256_ALGORITHM;
            break;
        case OE_HASH_TYPE_SHA512:
            pad_info.pszAlgId = BCRYPT_SHA512_ALGORITHM;
            break;
        default:
            OE_RAISE_MSG(
                OE_INVALID_PARAMETER,
                "Undefined oe_hash_type_t %x\n",
                hash_type);
            break;
    }

    NTSTATUS status = BCryptVerifySignature(
        impl->pkey,
        &pad_info,
        (PUCHAR)hash_data,
        (ULONG)hash_size,
        (PUCHAR)signature,
        (ULONG)signature_size,
        BCRYPT_PAD_PKCS1 /* Default type used by SGX RSA signatures */
    );

    if (!BCRYPT_SUCCESS(status))
        OE_RAISE_MSG(
            OE_CRYPTO_ERROR,
            "BCryptVerifySignature failed (err=%#x)\n",
            status);

    result = OE_OK;

done:
    return result;
}
