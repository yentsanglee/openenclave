// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/* Nest mbedtls header includes with required corelibc defines */
// clang-format off
#include "mbedtls_corelibc_defs.h"
#include <mbedtls/pk.h>
#include <mbedtls/cmac.h>
#include "mbedtls_corelibc_undef.h"
// clang-format on

#include <openenclave/enclave.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/cmac.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>

struct _oe_aes_cmac_context
{
    mbedtls_cipher_context_t ctx;
};

oe_result_t oe_aes_cmac_sign(
    const uint8_t* key,
    size_t key_size,
    const uint8_t* message,
    size_t message_length,
    oe_aes_cmac_t* aes_cmac)
{
    oe_result_t result = OE_UNEXPECTED;
    const mbedtls_cipher_info_t* info = NULL;
    size_t key_size_bits = key_size * 8;

    if (aes_cmac == NULL)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (key_size_bits != 128)
        OE_RAISE(OE_UNSUPPORTED);

    info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
    if (info == NULL)
        OE_RAISE(OE_FAILURE);

    oe_secure_zero_fill(aes_cmac->impl, sizeof(*aes_cmac));

    if (mbedtls_cipher_cmac(
            info,
            key,
            key_size_bits,
            message,
            message_length,
            (uint8_t*)aes_cmac->impl) != 0)
        OE_RAISE(OE_FAILURE);

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_aes_cmac_init(
    oe_aes_cmac_context_t** context,
    const uint8_t* key,
    size_t key_size)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_aes_cmac_context_t* ctx = NULL;
    const mbedtls_cipher_info_t* info = NULL;

    if (!context || !key)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (key_size != 16)
        OE_RAISE(OE_UNSUPPORTED);

    ctx = (oe_aes_cmac_context_t*)oe_malloc(sizeof(*ctx));
    if (!ctx)
        OE_RAISE(OE_OUT_OF_MEMORY);

    info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
    if (info == NULL)
        OE_RAISE(OE_FAILURE);

    mbedtls_cipher_init(&ctx->ctx);
    if (mbedtls_cipher_setup(&ctx->ctx, info) != 0)
    {
        mbedtls_cipher_free(&ctx->ctx);
        OE_RAISE(OE_FAILURE);
    }

    if (mbedtls_cipher_cmac_starts(&ctx->ctx, key, key_size * 8) != 0)
    {
        mbedtls_cipher_free(&ctx->ctx);
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

oe_result_t oe_aes_cmac_update(
    oe_aes_cmac_context_t* context,
    const uint8_t* message,
    size_t message_length)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!context || !message)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (mbedtls_cipher_cmac_update(&context->ctx, message, message_length) != 0)
        OE_RAISE(OE_FAILURE);

done:
    return result;
}

oe_result_t oe_aes_cmac_final(
    oe_aes_cmac_context_t* context,
    oe_aes_cmac_t* aes_cmac)
{
    oe_result_t result = OE_UNEXPECTED;

    if (!context || !aes_cmac)
        OE_RAISE(OE_INVALID_PARAMETER);

    oe_secure_zero_fill(aes_cmac->impl, sizeof(*aes_cmac));

    if (mbedtls_cipher_cmac_finish(&context->ctx, (uint8_t*)aes_cmac->impl) !=
        0)
        OE_RAISE(OE_FAILURE);

done:
    return result;
}

oe_result_t oe_aes_cmac_free(oe_aes_cmac_context_t* context)
{
    if (context)
    {
        mbedtls_cipher_free(&context->ctx);
        oe_free(context);
    }
    return OE_OK;
}
