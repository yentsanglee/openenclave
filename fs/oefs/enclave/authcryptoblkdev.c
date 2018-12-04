// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <mbedtls/aes.h>
#include <mbedtls/cmac.h>
#include <mbedtls/gcm.h>
#include <mbedtls/sha256.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"
#include "common.h"
#include "sha.h"
#include "utils.h"

#define AES_GCM_IV_SIZE 12

#define TAGS_PER_BLOCK (OEFS_BLOCK_SIZE / sizeof(tag_t))

#define KEY_BITS 256

typedef struct _tag
{
    uint8_t data[16];
} tag_t;

typedef struct _blkdev
{
    oefs_blkdev_t base;
    volatile uint64_t ref_count;
    size_t nblks;
    uint8_t key[OEFS_KEY_SIZE];
    oefs_blkdev_t* next;
    tag_t* tags;
    uint8_t* dirty;
    bool is_dirty;
} blkdev_t;

static int _generate_initialization_vector(
    const uint8_t key[OEFS_KEY_SIZE],
    uint64_t blkno,
    uint8_t iv[AES_GCM_IV_SIZE])
{
    int ret = -1;
    uint8_t in[16];
    uint8_t out[16];
    oefs_sha256_t khash;
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    memset(iv, 0x00, AES_GCM_IV_SIZE);

    /* The input buffer contains the block number followed by zeros. */
    memset(in, 0, sizeof(in));
    memcpy(in, &blkno, sizeof(blkno));

    /* Compute the hash of the key. */
    if (oefs_sha256(&khash, key, OEFS_KEY_SIZE) != 0)
        goto done;

    /* Use the hash of the key as the key. */
    if (mbedtls_aes_setkey_enc(&aes, khash.u.bytes, sizeof(khash) * 8) != 0)
        goto done;

    /* Encrypt the buffer with the hash of the key, yielding the IV. */
    if (mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, in, out) != 0)
        goto done;

    /* Use the first 12 bytes of the 16-byte buffer. */

    memcpy(iv, out, AES_GCM_IV_SIZE);

    ret = 0;

done:

    mbedtls_aes_free(&aes);
    return ret;
}

static int _encrypt(
    const uint8_t key[OEFS_KEY_SIZE],
    uint32_t blkno,
    const uint8_t in[OEFS_BLOCK_SIZE],
    uint8_t out[OEFS_BLOCK_SIZE],
    tag_t* tag)
{
    int rc = -1;
    mbedtls_gcm_context gcm;
    uint8_t iv[AES_GCM_IV_SIZE];

    mbedtls_gcm_init(&gcm);

    memset(tag, 0, sizeof(tag_t));

    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, KEY_BITS) != 0)
        goto done;

    if (_generate_initialization_vector(key, blkno, iv) != 0)
        goto done;

    if (mbedtls_gcm_crypt_and_tag(
            &gcm,
            MBEDTLS_GCM_ENCRYPT,
            OEFS_BLOCK_SIZE,
            iv,
            sizeof(iv),
            NULL,
            0,
            in,
            out,
            sizeof(tag_t),
            (unsigned char*)tag) != 0)
    {
        goto done;
    }

    rc = 0;

done:

    mbedtls_gcm_free(&gcm);

    return rc;
}

static int _decrypt(
    const uint8_t key[OEFS_KEY_SIZE],
    uint32_t blkno,
    const tag_t* tag,
    const uint8_t in[OEFS_BLOCK_SIZE],
    uint8_t out[OEFS_BLOCK_SIZE])
{
    int rc = -1;
    uint8_t iv[AES_GCM_IV_SIZE];
    mbedtls_gcm_context gcm;

    mbedtls_gcm_init(&gcm);

    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, KEY_BITS) != 0)
        goto done;

    if (_generate_initialization_vector(key, blkno, iv) != 0)
        goto done;

    if (mbedtls_gcm_auth_decrypt(
            &gcm,
            OEFS_BLOCK_SIZE,
            iv,
            sizeof(iv),
            NULL,
            0,
            (const unsigned char*)tag,
            sizeof(tag_t),
            in,
            out) != 0)
    {
        goto done;
    }

    rc = 0;

done:

    mbedtls_gcm_free(&gcm);

    return rc;
}

/* Write the tags just after the data blocks. */
static int _write_tags(blkdev_t* dev)
{
    int ret = -1;
    const oefs_blk_t* p = (const oefs_blk_t*)dev->tags;
    size_t n = dev->nblks / TAGS_PER_BLOCK;

    /* If any blocks are dirty. */
    if (dev->is_dirty)
    {
        /* For each hash page. */
        for (size_t i = 0; i < n; i++)
        {
            bool dirty = false;

            /* Check whether this hash page is dirty. */
            {
                size_t first = i * TAGS_PER_BLOCK;
                size_t last = first + TAGS_PER_BLOCK;

                uint64_t* p = (uint64_t*)&dev->dirty[first];
                uint64_t* end = (uint64_t*)&dev->dirty[last];

                while (p != end && *p == '\0')
                    p++;

                if (p != end)
                {
                    dirty = true;
                    memset(p, 0, (end - p) * sizeof(uint64_t));
                }
            }

            /* Write the hash page if any of its blocks are dirty. */
            if (dirty)
            {
                size_t off = dev->nblks;

                if (dev->next->put(dev->next, i + off, p) != 0)
                    goto done;
            }

            p++;
        }

        dev->is_dirty = false;
    }

    ret = 0;

done:
    return ret;
}

/* Read the tags just after the data blocks. */
static int _read_tags(blkdev_t* dev)
{
    int ret = -1;
    oefs_blk_t* p = (oefs_blk_t*)dev->tags;
    size_t n = dev->nblks / TAGS_PER_BLOCK;

    for (size_t i = 0; i < n; i++)
    {
        size_t off = dev->nblks;

        if (dev->next->get(dev->next, i + off, p) != 0)
            goto done;

        dev->dirty[i] = 0;

        p++;
    }

    ret = 0;

done:
    return ret;
}

static int _auth_crypto_blkdev_release(oefs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        goto done;

    if (oe_atomic_decrement(&dev->ref_count) == 0)
    {
        if (_write_tags(dev) != 0)
            goto done;

        dev->next->release(dev->next);
        free(dev->tags);
        free(dev->dirty);
        free(dev);
    }

    ret = 0;

done:
    return ret;
}

static int _auth_crypto_blkdev_get(
    oefs_blkdev_t* blkdev,
    uint32_t blkno,
    oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    oefs_blk_t encrypted;

    if (!dev || !blk || blkno >= dev->nblks)
        goto done;

    /* Delegate to the next block device in the chain. */
    if (dev->next->get(dev->next, blkno, &encrypted) != 0)
        goto done;

    /* Decrypt the block */
    if (_decrypt(
            dev->key,
            blkno,
            &dev->tags[blkno],
            (const uint8_t*)&encrypted,
            (uint8_t*)blk) != 0)
    {
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _auth_crypto_blkdev_put(
    oefs_blkdev_t* blkdev,
    uint32_t blkno,
    const oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    oefs_blk_t encrypted;
    tag_t tag;

    assert(blkno < dev->nblks);

    if (!dev || !blk || blkno >= dev->nblks)
        goto done;

    /* Encrypt the block */
    if (_encrypt(
            dev->key, blkno, (const uint8_t*)blk, (uint8_t*)&encrypted, &tag) !=
        0)
    {
        goto done;
    }

    /* Delegate to the next block dev in the chain. */
    if (dev->next->put(dev->next, blkno, &encrypted) != 0)
        goto done;

    /* Save the tag. */
    dev->tags[blkno] = tag;

    /* Set the dirty byte for this block's tag. */
    dev->dirty[blkno] = 1;
    dev->is_dirty = true;

    ret = 0;

done:

    return ret;
}

static int _auth_crypto_blkdev_begin(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* blkdev = (blkdev_t*)d;

    if (!blkdev || !blkdev->next)
        goto done;

    if (blkdev->next->begin(blkdev->next) != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _auth_crypto_blkdev_end(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        goto done;

    if (dev->next->end(dev->next) != 0)
        goto done;

    if (_write_tags(dev) != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _auth_crypto_blkdev_add_ref(oefs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        goto done;

    oe_atomic_increment(&dev->ref_count);

    ret = 0;

done:
    return ret;
}

int oefs_auth_crypto_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks)
{
    int ret = -1;
    size_t ntags;

    if (!extra_nblks)
        goto done;

    /* Calculate the number of authentication tags. */
    ntags = nblks;

    /* Calculate the number of blocks needed by a Merkle hash tree. */
    *extra_nblks =
        oefs_round_to_multiple(ntags, TAGS_PER_BLOCK) / TAGS_PER_BLOCK;

    ret = 0;

done:
    return ret;
}

int oefs_auth_crypto_blkdev_open(
    oefs_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    const uint8_t key[OEFS_KEY_SIZE],
    oefs_blkdev_t* next)
{
    int ret = -1;
    blkdev_t* dev = NULL;
    tag_t* tags = NULL;
    uint8_t* dirty = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !nblks || !next)
        goto done;

    if (!(dev = calloc(1, sizeof(blkdev_t))))
        goto done;

    /* Allocates tags array (must be a multiple of the block size). */
    {
        size_t size = sizeof(nblks * sizeof(tags));
        size = oefs_round_to_multiple(size, OEFS_BLOCK_SIZE);

        if (!(tags = calloc(nblks, size)))
            goto done;
    }

    if (!(dirty = calloc(nblks, sizeof(uint8_t))))
        goto done;

    dev->base.get = _auth_crypto_blkdev_get;
    dev->base.put = _auth_crypto_blkdev_put;
    dev->base.begin = _auth_crypto_blkdev_begin;
    dev->base.end = _auth_crypto_blkdev_end;
    dev->base.add_ref = _auth_crypto_blkdev_add_ref;
    dev->base.release = _auth_crypto_blkdev_release;
    dev->ref_count = 1;
    dev->nblks = nblks;
    dev->next = next;
    dev->tags = tags;
    dev->dirty = dirty;
    memcpy(dev->key, key, OEFS_KEY_SIZE);

    if (initialize)
    {
        /* Set all the dirty bits so all nodes will be written. */
        memset(dev->dirty, 1, dev->nblks);
        dev->is_dirty = true;

        /* Write out zero blocks. */
        {
            oefs_blk_t zero_blk;

            /* Initialize the zero-filled block. */
            memset(&zero_blk, 0, sizeof(zero_blk));

            /* Write all the data blocks. */
            for (size_t i = 0; i < nblks; i++)
            {
                if (dev->base.put(&dev->base, 0, &zero_blk) != 0)
                    goto done;
            }
        }

        /* Write the tags to the next device. */
        if (_write_tags(dev) != 0)
            goto done;
    }
    else
    {
        if (_read_tags(dev) != 0)
            goto done;
    }

    next->add_ref(next);
    *blkdev = &dev->base;
    dev = NULL;
    tags = NULL;
    dirty = NULL;

    ret = 0;

done:

    if (dev)
        free(dev);

    if (tags)
        free(tags);

    if (dirty)
        free(dirty);

    return ret;
}
