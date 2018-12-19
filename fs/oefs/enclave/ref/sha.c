#include "sha.h"
#include <mbedtls/sha256.h>
#include <openenclave/internal/defs.h>
#include <stdio.h>

OE_STATIC_ASSERT(
    sizeof(mbedtls_sha256_context) <= sizeof(oefs_sha256_context_t));

int oefs_sha256_init(oefs_sha256_context_t* context)
{
#if !defined(FAKE)
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)context;

    mbedtls_sha256_init(ctx);

    if (mbedtls_sha256_starts_ret(ctx, 0) != 0)
        return -1;
#endif

    return 0;
}

int oefs_sha256_update(
    oefs_sha256_context_t* context,
    const void* data,
    size_t size)
{
#if !defined(FAKE)
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)context;

    if (mbedtls_sha256_update_ret(ctx, (const uint8_t*)data, size) != 0)
        return -1;
#endif

    return 0;
}

int oefs_sha256_finish(oefs_sha256_context_t* context, oefs_sha256_t* hash)
{
#if defined(FAKE)
    /* Generate a fake hash for performance comparisons. */
    memset(hash->data, 0xfa, sizeof(oefs_sha256_t));
#else
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)context;

    if (mbedtls_sha256_finish_ret(ctx, hash->data) != 0)
        return -1;
#endif

    return 0;
}

void oefs_sha256_release(const oefs_sha256_context_t* context)
{
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)context;

    mbedtls_sha256_free(ctx);
}

int oefs_sha256(oefs_sha256_t* hash, const void* data, size_t size)
{
    int ret = -1;
    oefs_sha256_context_t ctx;

    if (hash)
        memset(hash, 0, sizeof(oefs_sha256_t));

    if (!hash || !data)
        goto done;

    oefs_sha256_init(&ctx);

    if (oefs_sha256_update(&ctx, data, size) != 0)
        goto done;

    if (oefs_sha256_finish(&ctx, (oefs_sha256_t*)hash->data) != 0)
        goto done;

    oefs_sha256_release(&ctx);

    ret = 0;

done:
    return ret;
}

void oefs_sha256_dump(const oefs_sha256_t* hash)
{
    for (size_t i = 0; i < sizeof(oefs_sha256_t); i++)
    {
        uint8_t byte = hash->data[i];
        printf("%02x", byte);
    }

    printf("\n");
}

void oefs_sha256_context_dump(const oefs_sha256_context_t* context)
{
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)context;

    printf("total[0]=%u\n", ctx->total[0]);
    printf("total[1]=%u\n", ctx->total[1]);

    for (size_t i = 0; i < sizeof(ctx->state) / sizeof(ctx->state[0]); i++)
    {
        printf("state{%08x}\n", ctx->state[i]);
    }
}

const char* oefs_sha256_str(const oefs_sha256_t* hash, oefs_sha256_str_t* str)
{
    char* p = str->data;

    for (size_t i = 0; i < sizeof(oefs_sha256_t); i++)
    {
        uint8_t byte = hash->data[i];
        snprintf(p, 3, "%02x", byte);
        p += 2;
    }

    return str->data;
}
