// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/* Nest mbedtls header includes with required corelibc defines */

#include <openenclave/bits/types.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/sha.h>
#include <openssl/evp.h>

typedef struct _oe_sha256_context_impl
{
    EVP_MD_CTX *ctx;
    uint64_t impl[15];
} oe_sha256_context_impl_t;

OE_STATIC_ASSERT(
    sizeof(oe_sha256_context_impl_t) <= sizeof(oe_sha256_context_t));

oe_result_t oe_sha256_init(oe_sha256_context_t* context)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_sha256_context_impl_t* impl = (oe_sha256_context_impl_t*)context;

    if (!context)
        OE_RAISE(OE_INVALID_PARAMETER);

    if((impl->ctx = EVP_MD_CTX_create()) == NULL)
    {
        OE_RAISE(OE_UNEXPECTED);
    }

    if(1 != EVP_DigestInit_ex(impl->ctx, EVP_sha256(), NULL))
    {
        OE_RAISE(OE_UNEXPECTED);
    }

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_sha256_update(
    oe_sha256_context_t* context,
    const void* data,
    size_t size)
{
    oe_result_t result = OE_INVALID_PARAMETER;
    oe_sha256_context_impl_t* impl = (oe_sha256_context_impl_t*)context;

    if (!context || !data)
        OE_RAISE(OE_INVALID_PARAMETER);

    if(1 != EVP_DigestUpdate(impl->ctx, data, size))
    {
        OE_RAISE(OE_UNEXPECTED);
    }

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_sha256_final(oe_sha256_context_t* context, OE_SHA256* sha256)
{
    oe_result_t result = OE_INVALID_PARAMETER;
    oe_sha256_context_impl_t* impl = (oe_sha256_context_impl_t*)context;
    unsigned int final_size = sizeof(OE_SHA256);

    if (!context || !sha256)
        OE_RAISE(OE_INVALID_PARAMETER);

    if(1 != EVP_DigestFinal_ex(impl->ctx, sha256->buf, &final_size))
    {
        OE_RAISE(OE_UNEXPECTED);
    }

    if (final_size != sizeof(OE_SHA256))
    {
        OE_RAISE(OE_UNEXPECTED);
    }

    EVP_MD_CTX_destroy(impl->ctx);

    result = OE_OK;

done:
    return result;
}
