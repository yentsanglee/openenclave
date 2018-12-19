// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <mbedtls/aes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"
#include "common.h"
#include "sha256.h"

#define SHA256_SIZE 32

#define IV_SIZE 16

typedef struct _blkdev
{
    oefs_blkdev_t base;
    volatile uint64_t ref_count;
    uint8_t key[OEFS_KEY_SIZE];
    mbedtls_aes_context enc_aes;
    mbedtls_aes_context dec_aes;
    oefs_blkdev_t* next;
} blkdev_t;

static int _generate_initialization_vector(
    const uint8_t key[OEFS_KEY_SIZE],
    uint64_t blkno,
    uint8_t iv[IV_SIZE])
{
    int ret = -1;
    uint8_t buf[IV_SIZE];
    sha256_t khash;
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    memset(iv, 0, sizeof(IV_SIZE));

    /* The input buffer contains the block number followed by zeros. */
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &blkno, sizeof(blkno));

    /* Hash the key (pad out to 64-bytes first). */
    {
        struct
        {
            uint8_t key[OEFS_KEY_SIZE];
            uint8_t padding[64 - OEFS_KEY_SIZE];
        } buf;
        OE_STATIC_ASSERT(sizeof(buf) == 64);

        memcpy(buf.key, key, sizeof(buf.key));
        memset(buf.padding, 0, sizeof(buf.padding));

        /* Compute the hash of the key. */
        if (sha256(&khash, &buf, sizeof(buf)) != 0)
            goto done;
    }

    /* Create a SHA-256 hash of the key. */
    if (mbedtls_aes_setkey_enc(&aes, khash.u.data, sizeof(khash) * 8) != 0)
        goto done;

    /* Encrypt the buffer with the hash of the key, yielding the IV. */
    if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, buf, iv) != 0)
        goto done;

    ret = 0;

done:

    mbedtls_aes_free(&aes);
    return ret;
}

static int _crypt(
    mbedtls_aes_context* aes,
    int mode, /* MBEDTLS_AES_ENCRYPT or MBEDTLS_AES_DECRYPT */
    const uint8_t key[OEFS_KEY_SIZE],
    uint32_t blkno,
    const uint8_t in[OEFS_BLOCK_SIZE],
    uint8_t out[OEFS_BLOCK_SIZE])
{
    int rc = -1;
    uint8_t iv[IV_SIZE];

    /* Generate an initialization vector for this block number. */
    if (_generate_initialization_vector(key, blkno, iv) != 0)
        goto done;

    /* Encypt the data. */
    if (mbedtls_aes_crypt_cbc(aes, mode, OEFS_BLOCK_SIZE, iv, in, out) != 0)
        goto done;

    rc = 0;

done:

    return rc;
}

static int _crypto_blkdev_release(oefs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    if (oe_atomic_decrement(&device->ref_count) == 0)
    {
        mbedtls_aes_free(&device->enc_aes);
        mbedtls_aes_free(&device->dec_aes);
        device->next->release(device->next);
        free(device);
    }

    ret = 0;

done:
    return ret;
}

static int _crypto_blkdev_get(
    oefs_blkdev_t* dev,
    uint32_t blkno,
    oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    oefs_blk_t encrypted;

    if (!device || !blk)
        goto done;

    /* Delegate to the next block device in the chain. */
    if (device->next->get(device->next, blkno, &encrypted) != 0)
        goto done;

    /* Decrypt the block */
    if (_crypt(
            &device->dec_aes,
            MBEDTLS_AES_DECRYPT,
            device->key,
            blkno,
            encrypted.u.data,
            blk->u.data) != 0)
    {
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _crypto_blkdev_put(
    oefs_blkdev_t* dev,
    uint32_t blkno,
    const oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    oefs_blk_t encrypted;

    if (!device || !blk)
        goto done;

    /* Encrypt the block */
    if (_crypt(
            &device->enc_aes,
            MBEDTLS_AES_ENCRYPT,
            device->key,
            blkno,
            blk->u.data,
            encrypted.u.data) != 0)
    {
        goto done;
    }

    /* Delegate to the next block device in the chain. */
    if (device->next->put(device->next, blkno, &encrypted) != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _crypto_blkdev_begin(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        goto done;

    if (dev->next->begin(dev->next) != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _crypto_blkdev_end(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        goto done;

    if (dev->next->end(dev->next) != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _crypto_blkdev_add_ref(oefs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    oe_atomic_increment(&device->ref_count);

    ret = 0;

done:
    return ret;
}

int oefs_crypto_blkdev_open(
    oefs_blkdev_t** blkdev,
    const uint8_t key[OEFS_KEY_SIZE],
    oefs_blkdev_t* next)
{
    int ret = -1;
    blkdev_t* device = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !next)
        goto done;

    if (!(device = calloc(1, sizeof(blkdev_t))))
        goto done;

    device->base.get = _crypto_blkdev_get;
    device->base.put = _crypto_blkdev_put;
    device->base.begin = _crypto_blkdev_begin;
    device->base.end = _crypto_blkdev_end;
    device->base.add_ref = _crypto_blkdev_add_ref;
    device->base.release = _crypto_blkdev_release;
    device->ref_count = 1;
    device->next = next;
    memcpy(device->key, key, OEFS_KEY_SIZE);
    mbedtls_aes_init(&device->enc_aes);
    mbedtls_aes_init(&device->dec_aes);

    /* Initialize an AES context for encrypting. */
    if (mbedtls_aes_setkey_enc(&device->enc_aes, key, OEFS_KEY_SIZE * 8) != 0)
    {
        mbedtls_aes_free(&device->enc_aes);
        goto done;
    }

    /* Initialize an AES context for decrypting. */
    if (mbedtls_aes_setkey_dec(&device->dec_aes, key, OEFS_KEY_SIZE * 8) != 0)
    {
        mbedtls_aes_free(&device->enc_aes);
        mbedtls_aes_free(&device->dec_aes);
        goto done;
    }

    next->add_ref(next);

    *blkdev = &device->base;
    device = NULL;

    ret = 0;

done:

    if (device)
        free(device);

    return ret;
}
