// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/* Nest mbedtls header includes with required corelibc defines */
// clang-format off
#include "mbedtls_corelibc_defs.h"
#include <mbedtls/cipher.h>
#include <mbedtls/gcm.h>
#include "mbedtls_corelibc_undef.h"
// clang-format on

#include <openenclave/bits/safecrt.h>
#include <openenclave/enclave.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include <openenclave/internal/aes.h>

struct _oe_aes_gcm_context
{
    mbedtls_gcm_context ctx;
};

struct _oe_aes_ctr_context
{
    mbedtls_cipher_context_t ctx;
};

oe_result_t oe_aes_gcm_init(
    oe_aes_gcm_context_t** context,
    const uint8_t* key,
    size_t key_size,
    const uint8_t* iv,
    size_t iv_size,
    const uint8_t* aad,
    size_t aad_size,
    bool encrypt)
{
    oe_aes_gcm_context_t* ctx = NULL;
    oe_result_t result = OE_UNEXPECTED;

    if (!context || !key || !iv)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (key_size != 16 || iv_size != 12)
        OE_RAISE(OE_UNSUPPORTED);

    ctx = (oe_aes_gcm_context_t*)oe_malloc(sizeof(*ctx));
    if (!ctx)
        OE_RAISE(OE_OUT_OF_MEMORY);

    mbedtls_gcm_init(&ctx->ctx);

    if (mbedtls_gcm_setkey(
            &ctx->ctx,
            MBEDTLS_CIPHER_ID_AES,
            key,
            (unsigned int)key_size * 8) != 0)
    {
        mbedtls_gcm_free(&ctx->ctx);
        OE_RAISE(OE_FAILURE);
    }

    if (mbedtls_gcm_starts(
            &ctx->ctx,
            encrypt ? MBEDTLS_GCM_ENCRYPT : MBEDTLS_GCM_DECRYPT,
            iv,
            iv_size,
            aad,
            aad_size) != 0)
    {
        mbedtls_gcm_free(&ctx->ctx);
        OE_RAISE(OE_FAILURE);
    }

    *context = ctx;
    ctx = NULL;
    result = OE_OK;

done:
    if (ctx != NULL)
        oe_free(ctx);

    return result;
}

oe_result_t oe_aes_gcm_update(
    oe_aes_gcm_context_t* context,
    const uint8_t* input,
    uint8_t* output,
    size_t size)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!context || !input || !output)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (mbedtls_gcm_update(&context->ctx, size, input, output) != 0)
        OE_RAISE(OE_FAILURE);

done:
    return result;
}

oe_result_t oe_aes_gcm_final(
    oe_aes_gcm_context_t* context,
    oe_aes_gcm_tag_t* tag)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!context || !tag)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (mbedtls_gcm_finish(&context->ctx, tag->buf, sizeof(tag->buf)) != 0)
        OE_RAISE(OE_FAILURE);

done:
    return result;
}

oe_result_t oe_aes_gcm_free(oe_aes_gcm_context_t* context)
{
    if (context)
    {
        mbedtls_gcm_free(&context->ctx);
        oe_free(context);
    }

    return OE_OK;
}

oe_result_t oe_aes_ctr_init(
    oe_aes_ctr_context_t** context,
    const uint8_t* key,
    size_t key_size,
    const uint8_t* iv,
    size_t iv_size,
    bool encrypt)
{
    oe_aes_ctr_context_t* ctx = NULL;
    oe_result_t result = OE_UNEXPECTED;
    const mbedtls_cipher_info_t* info = NULL;

    if (!context || !key || !iv || key_size != 16 || iv_size != 16)
        OE_RAISE(OE_INVALID_PARAMETER);

    ctx = (oe_aes_ctr_context_t*)oe_malloc(sizeof(*ctx));
    if (!ctx)
        OE_RAISE(OE_OUT_OF_MEMORY);

    mbedtls_cipher_init(&ctx->ctx);

    info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_CTR);
    if (!info)
        OE_RAISE(OE_OUT_OF_MEMORY);

    if (mbedtls_cipher_setup(&ctx->ctx, info) != 0)
        OE_RAISE(OE_FAILURE);

    if (mbedtls_cipher_setkey(
            &ctx->ctx,
            key,
            (int)key_size * 8,
            encrypt ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT) != 0)
        OE_RAISE(OE_FAILURE);

    if (mbedtls_cipher_set_iv(&ctx->ctx, iv, iv_size) != 0)
        OE_RAISE(OE_FAILURE);

    *context = ctx;
    ctx = NULL;
    result = OE_OK;

done:
    if (ctx != NULL)
    {
        mbedtls_cipher_free(&ctx->ctx);
        oe_free(ctx);
    }

    return result;
}

oe_result_t oe_aes_ctr_crypt(
    oe_aes_ctr_context_t* context,
    const uint8_t* input,
    uint8_t* output,
    size_t size)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!context || !input || !output)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (mbedtls_cipher_update(&context->ctx, input, size, output, &size) != 0)
        OE_RAISE(OE_FAILURE);

done:
    return result;
}

oe_result_t oe_aes_ctr_free(oe_aes_ctr_context_t* context)
{
    if (context)
    {
        mbedtls_cipher_free(&context->ctx);
        oe_free(context);
    }
    return OE_OK;
}
