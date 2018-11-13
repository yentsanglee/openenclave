// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atomic.h"
#include "blkdev.h"
#include "sha.h"

#define SHA256_SIZE 32

#define IV_SIZE 16

typedef struct _blkdev
{
    fs_blkdev_t base;
    volatile uint64_t ref_count;
    uint8_t key[FS_KEY_SIZE];
    mbedtls_aes_context enc_aes;
    mbedtls_aes_context dec_aes;
    fs_blkdev_t* next;
} blkdev_t;

static int _generate_initialization_vector(
    const uint8_t key[FS_KEY_SIZE],
    uint64_t blkno,
    uint8_t iv[IV_SIZE])
{
    int ret = -1;
    uint8_t buf[IV_SIZE];
    fs_sha256_t khash;
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    memset(iv, 0, sizeof(IV_SIZE));

    /* The input buffer contains the block number followed by zeros. */
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &blkno, sizeof(blkno));

    /* Compute the hash of the key. */
    if (fs_sha256(&khash, key, FS_KEY_SIZE) != 0)
        goto done;

    /* Create a SHA-256 hash of the key. */
    if (mbedtls_aes_setkey_enc(&aes, khash.u.bytes, sizeof(khash) * 8) != 0)
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
    const uint8_t key[FS_KEY_SIZE],
    uint32_t blkno,
    const uint8_t in[FS_BLOCK_SIZE],
    uint8_t out[FS_BLOCK_SIZE])
{
    int rc = -1;
    uint8_t iv[IV_SIZE];

    /* Generate an initialization vector for this block number. */
    if (_generate_initialization_vector(key, blkno, iv) != 0)
        goto done;

    /* Encypt the data. */
    if (mbedtls_aes_crypt_cbc(aes, mode, FS_BLOCK_SIZE, iv, in, out) != 0)
        goto done;

    rc = 0;

done:

    return rc;
}

static int _blkdev_release(fs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    if (fs_atomic_decrement(&device->ref_count) == 0)
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

static int _blkdev_get(fs_blkdev_t* dev, uint32_t blkno, fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    fs_blk_t encrypted;

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
            encrypted.data,
            blk->data) != 0)
    {
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _blkdev_put(fs_blkdev_t* dev, uint32_t blkno, const fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;
    fs_blk_t encrypted;

    if (!device || !blk)
        goto done;

    /* Encrypt the block */
    if (_crypt(
            &device->enc_aes,
            MBEDTLS_AES_ENCRYPT,
            device->key,
            blkno,
            blk->data,
            encrypted.data) != 0)
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

static int _blkdev_begin(fs_blkdev_t* d)
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

static int _blkdev_end(fs_blkdev_t* d)
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

static int _blkdev_add_ref(fs_blkdev_t* dev)
{
    int ret = -1;
    blkdev_t* device = (blkdev_t*)dev;

    if (!device)
        goto done;

    fs_atomic_increment(&device->ref_count);

    ret = 0;

done:
    return ret;
}

int fs_crypto_blkdev_open(
    fs_blkdev_t** blkdev,
    const uint8_t key[FS_KEY_SIZE],
    fs_blkdev_t* next)
{
    int ret = -1;
    blkdev_t* device = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !next)
        goto done;

    if (!(device = calloc(1, sizeof(blkdev_t))))
        goto done;

    device->base.get = _blkdev_get;
    device->base.put = _blkdev_put;
    device->base.begin = _blkdev_begin;
    device->base.end = _blkdev_end;
    device->base.add_ref = _blkdev_add_ref;
    device->base.release = _blkdev_release;
    device->ref_count = 1;
    device->next = next;
    memcpy(device->key, key, FS_KEY_SIZE);
    mbedtls_aes_init(&device->enc_aes);
    mbedtls_aes_init(&device->dec_aes);

    /* Initialize an AES context for encrypting. */
    if (mbedtls_aes_setkey_enc(&device->enc_aes, key, FS_KEY_SIZE * 8) != 0)
    {
        mbedtls_aes_free(&device->enc_aes);
        goto done;
    }

    /* Initialize an AES context for decrypting. */
    if (mbedtls_aes_setkey_dec(&device->dec_aes, key, FS_KEY_SIZE * 8) != 0)
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
