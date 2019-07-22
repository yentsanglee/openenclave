// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _HOST_KEY_H
#define _HOST_KEY_H

#include <openenclave/bits/result.h>
#include <openenclave/bits/types.h>
#include <openenclave/internal/crypto/hash.h>
#include "bcrypt.h"

typedef struct _oe_bcrypt_key
{
    uint64_t magic;
    BCRYPT_KEY_HANDLE pkey;
} oe_private_key_t, oe_public_key_t, oe_bcrypt_key_t;

typedef struct _oe_bcrypt_key_format
{
    LPCSTR encoding;
    BCRYPT_ALG_HANDLE key_algorithm;
    LPCWSTR key_blob_type;
} oe_bcrypt_key_format_t;

typedef struct _oe_bcrypt_padding_info
{
    uint32_t type;
    void* config;
} oe_bcrypt_padding_info_t;

// typedef oe_result_t (
//    *oe_private_key_write_pem_callback)(BIO* bio, EVP_PKEY* pkey);
//
bool oe_bcrypt_key_is_valid(const oe_bcrypt_key_t* impl, uint64_t magic);

oe_result_t oe_bcrypt_key_free(oe_bcrypt_key_t* key, uint64_t magic);

oe_result_t oe_bcrypt_key_get_blob(
    const oe_bcrypt_key_t* key,
    uint64_t magic,
    LPCWSTR blob_type,
    BYTE** blob_data,
    ULONG* blob_size);

oe_result_t oe_bcrypt_read_key_pem(
    const uint8_t* pem_data,
    size_t pem_size,
    oe_bcrypt_key_t* key,
    oe_bcrypt_key_format_t key_args,
    uint64_t magic);

oe_result_t oe_bcrypt_write_key_pem(
    const oe_bcrypt_key_t* key,
    oe_bcrypt_key_format_t key_args,
    uint64_t magic,
    uint8_t* pem_data,
    size_t* pem_size);

void oe_bcrypt_key_init(
    oe_bcrypt_key_t* key,
    BCRYPT_KEY_HANDLE* pkey,
    uint64_t magic);

// void oe_private_key_init(
//    oe_private_key_t* private_key,
//    EVP_PKEY* pkey,
//    uint64_t magic);
//
// oe_result_t oe_private_key_read_pem(
//    const uint8_t* pem_data,
//    size_t pem_size,
//    oe_private_key_t* key,
//    int key_type,
//    uint64_t magic);
//
// oe_result_t oe_public_key_read_pem(
//    const uint8_t* pem_data,
//    size_t pem_size,
//    oe_public_key_t* key,
//    int key_type,
//    uint64_t magic);
//
// oe_result_t oe_private_key_write_pem(
//    const oe_private_key_t* private_key,
//    uint8_t* data,
//    size_t* size,
//    oe_private_key_write_pem_callback private_key_write_pem_callback,
//    uint64_t magic);

oe_result_t oe_public_key_write_pem(
    const oe_public_key_t* key,
    uint8_t* data,
    size_t* size,
    uint64_t magic);

 oe_result_t oe_private_key_sign(
    const oe_private_key_t* private_key,
    const oe_bcrypt_padding_info_t* padding_info,
    const void* hash_data,
    size_t hash_size,
    uint8_t* signature,
    size_t* signature_size,
    uint64_t magic);

oe_result_t oe_public_key_verify(
    const oe_public_key_t* public_key,
    const oe_bcrypt_padding_info_t* padding_info,
    const void* hash_data,
    size_t hash_size,
    const uint8_t* signature,
    size_t signature_size,
    uint64_t magic);

#endif /* _HOST_KEY_H */
