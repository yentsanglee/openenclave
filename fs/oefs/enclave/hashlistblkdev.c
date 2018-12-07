// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/print.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"
#include "common.h"
#include "utils.h"
#include "sha.h"

#define ENABLE_TRACE_GOTO
#include "trace.h"

#define HASH_SIZE (sizeof(oefs_sha256_t))

#define HASHES_PER_BLOCK ((OEFS_BLOCK_SIZE / HASH_SIZE) - 1)

#define MAGIC 0x6ad82e3df77026f9

/* Block layout: [ header block ] [ hash blocks ] [ data blocks ]. */

typedef struct _header_block
{
    /* Magic number: MAGIC */
    uint64_t magic;

    /* The total number of data blocks in the system. */
    uint64_t nblks;

    /* The hash of hash_blocks[i].hash for i in [0:nblks) */
    oefs_sha256_t hash;

    uint8_t reserved[OEFS_BLOCK_SIZE - 8 - 8 - 32];
}
header_block_t;

OE_STATIC_ASSERT(sizeof(header_block_t) == OEFS_BLOCK_SIZE);

typedef struct _hash_block
{
    /* The hash of the hashes[] array. */
    oefs_sha256_t hash;

    /* The hash of data blocks. */
    oefs_sha256_t hashes[HASHES_PER_BLOCK];
}
hash_block_t;

OE_STATIC_ASSERT(sizeof(hash_block_t) == OEFS_BLOCK_SIZE);

typedef struct _blkdev
{
    oefs_blkdev_t base;
    volatile uint64_t ref_count;
    oefs_blkdev_t* next;

    header_block_t header_block;

    /* In-memory copy of the hash blocks. */
    hash_block_t* hash_blocks;
    size_t num_hash_blocks;

    /* The dirty hash blocks. */
    uint8_t* dirty_hash_blocks;

    /* True if any dirty_hash_blocks[] elements is non-zero. */
    bool have_dirty_hash_blocks;

} blkdev_t;

static int _update_hash_block(hash_block_t* hash_block)
{
    int ret = -1;
    oefs_sha256_context_t ctx;

    oefs_sha256_init(&ctx);

    for (size_t i = 0; i < HASHES_PER_BLOCK; i++)
    {
        const oefs_sha256_t* hash = &hash_block->hashes[i];

        if (oefs_sha256_update(&ctx, hash->data, HASH_SIZE) != 0)
            GOTO(done);
    }

    if (oefs_sha256_finish(&ctx, (oefs_sha256_t*)hash_block->hash.data) != 0)
        GOTO(done);

    ret = 0;

done:

    oefs_sha256_release(&ctx);

    return ret;
}

static int _update_header_block(blkdev_t* dev)
{
    int ret = -1;
    oefs_sha256_context_t ctx;

    oefs_sha256_init(&ctx);

    for (size_t i = 0; i < dev->num_hash_blocks; i++)
    {
        hash_block_t* hash_block = &dev->hash_blocks[i];
        const oefs_sha256_t* hash;

        if (dev->dirty_hash_blocks[i])
        {
            if (_update_hash_block(hash_block) != 0)
                GOTO(done);
        }

        hash = &hash_block->hash;

        if (oefs_sha256_update(&ctx, hash->data, HASH_SIZE) != 0)
            GOTO(done);
    }

    if (oefs_sha256_finish(&ctx, (oefs_sha256_t*)dev->header_block.hash.data) != 0)
        GOTO(done);

    ret = 0;

done:

    oefs_sha256_release(&ctx);

    return ret;
}

static void _set_hash(blkdev_t* dev, size_t blkno, const oefs_sha256_t* hash)
{
    uint32_t i = blkno / HASHES_PER_BLOCK;
    uint32_t j = blkno % HASHES_PER_BLOCK;

    oe_assert(blkno < dev->header_block.nblks);
    oe_assert(i < dev->num_hash_blocks);

    dev->hash_blocks[i].hashes[j] = *hash;
    dev->dirty_hash_blocks[i] = 1;
    dev->have_dirty_hash_blocks = true;
}

static int _flush_header_block(blkdev_t* dev)
{
    int ret = -1;
    size_t blkno = dev->header_block.nblks;

    if (dev->next->put(dev->next, blkno, (oefs_blk_t*)&dev->header_block) != 0)
        GOTO(done);

    return 0;

done:
    return ret;
}

static int _load_header_block(blkdev_t* dev)
{
    int ret = -1;
    size_t blkno = dev->header_block.nblks;

    if (dev->next->get(dev->next, blkno, (oefs_blk_t*)&dev->header_block) != 0)
        GOTO(done);

    return 0;

done:
    return ret;
}

static int _flush_hash_list(blkdev_t* dev)
{
    int ret = -1;
    size_t blkno;

    if (!dev->have_dirty_hash_blocks)
    {
        ret = 0;
        goto done;
    }

    /* Update the master hash. */
    if (_update_header_block(dev) != 0)
        GOTO(done);

    /* Flush the header. */
    if (_flush_header_block(dev) != 0)
        GOTO(done);

    /* Calculate the block number of the first hash block. */
    blkno = dev->header_block.nblks + 1;

    /* Flush the dirty hash blocks. */
    for (size_t i = 0; i < dev->num_hash_blocks; i++)
    {
        if (dev->dirty_hash_blocks[i])
        {
            oefs_blk_t* blk = (oefs_blk_t*)&dev->hash_blocks[i];

            if (dev->next->put(dev->next, blkno + i, blk) != 0)
                GOTO(done);

            dev->dirty_hash_blocks[i] = 0;
        }
    }

    dev->have_dirty_hash_blocks = false;

    ret = 0;

done:
    return ret;
}

static int _load_hash_list(blkdev_t* dev)
{
    int ret = -1;
    size_t blkno;
    hash_block_t* hash_blocks = NULL;
    uint8_t* dirty_hash_blocks = NULL;

    /* Load the header block. */
    {
        if (_load_header_block(dev) != 0)
            GOTO(done);

        if (dev->header_block.magic != MAGIC)
            GOTO(done);
    }

    /* Calculate dev->num_hash_blocks */
    dev->num_hash_blocks = oefs_round_to_multiple(
        dev->header_block.nblks, HASHES_PER_BLOCK) / HASHES_PER_BLOCK;

    /* Allocate the dev->hash_blocks[] array. */
    {
        size_t alloc_size = dev->num_hash_blocks * sizeof(hash_block_t);

        if (!(hash_blocks = malloc(alloc_size)))
            GOTO(done);

        dev->hash_blocks = hash_blocks;
    }

    /* Allocate the dev->dirty_hash_blocks[] array. */
    {
        size_t alloc_size = dev->num_hash_blocks * sizeof(uint8_t);

        if (!(dirty_hash_blocks = calloc(1, alloc_size)))
            GOTO(done);

        dev->dirty_hash_blocks = dirty_hash_blocks;
    }

    /* Calculate the block number of the first hash block. */
    blkno = dev->header_block.nblks + 1;

    /* Load each of the hash blocks. */
    for (size_t i = 0; i < dev->num_hash_blocks; i++)
    {
        hash_block_t* hash_block = &dev->hash_blocks[i];

        if (dev->next->get(dev->next, i + blkno, (oefs_blk_t*)hash_block) != 0)
            GOTO(done);
    }

    /* Verify the master hash. */
    {
        const oefs_sha256_t hash = dev->header_block.hash;

        if (_update_header_block(dev) != 0)
            GOTO(done);

        if (memcmp(&dev->header_block.hash, &hash, sizeof(hash)) != 0)
            GOTO(done);
    }

    hash_blocks = NULL;
    dirty_hash_blocks = NULL;

    ret = 0;

done:

    if (hash_blocks)
        free(hash_blocks);

    if (dirty_hash_blocks)
        free(dirty_hash_blocks);

    return ret;
}

/* Initialize a hash block whose data blocks are all zero-filled. */
static int _initialize_zero_hash_block(hash_block_t* hash_block)
{
    int ret = -1;
    oefs_blk_t zero_blk;
    oefs_sha256_t hash;

    memset(&zero_blk, 0, sizeof(oefs_blk_t));
    memset(hash_block, 0, sizeof(hash_block_t));

    if (oefs_sha256(&hash, &zero_blk, sizeof(zero_blk)) != 0)
        GOTO(done);

    for (size_t i = 0; i < HASHES_PER_BLOCK; i++)
        hash_block->hashes[i] = hash;

    if (_update_hash_block(hash_block) != 0)
        GOTO(done);

    ret = 0;

done:
    return ret;
}

static int _initialize_hash_list(blkdev_t* dev, size_t nblks)
{
    int ret = -1;
    hash_block_t* hash_blocks = NULL;
    uint8_t* dirty_hash_blocks = NULL;

    /* Initialize the header block. */
    {
        memset(&dev->header_block, 0, sizeof(dev->header_block));
        dev->header_block.magic = MAGIC;
        dev->header_block.nblks = nblks;
    }

    /* Calculate dev->num_hash_blocks */
    dev->num_hash_blocks = oefs_round_to_multiple(
        dev->header_block.nblks, HASHES_PER_BLOCK) / HASHES_PER_BLOCK;

    /* Allocate the dev->hash_blocks[] array. */
    {
        size_t alloc_size = dev->num_hash_blocks * sizeof(hash_block_t);

        if (!(hash_blocks = malloc(alloc_size)))
            GOTO(done);

        dev->hash_blocks = hash_blocks;
    }

    /* Allocate the dev->dirty_hash_blocks[] array. */
    {
        size_t alloc_size = dev->num_hash_blocks * sizeof(uint8_t);

        if (!(dirty_hash_blocks = calloc(1, alloc_size)))
            GOTO(done);

        dev->dirty_hash_blocks = dirty_hash_blocks;
    }

    /* Initialize each of the hash blocks. */
    {
        hash_block_t zero_hash_block;

        if (_initialize_zero_hash_block(&zero_hash_block) != 0)
            GOTO(done);

        for (size_t i = 0; i < dev->num_hash_blocks; i++)
            dev->hash_blocks[i] = zero_hash_block;
    }

    /* Initialize the master hash. */
    if (_update_header_block(dev) != 0)
        GOTO(done);

    hash_blocks = NULL;
    dirty_hash_blocks = NULL;

    ret = 0;

done:

    if (hash_blocks)
        free(hash_blocks);

    if (dirty_hash_blocks)
        free(dirty_hash_blocks);

    return ret;
}

static int _check_hash(blkdev_t* dev, uint32_t blkno, const oefs_sha256_t* hash)
{
    int ret = -1;
    uint32_t i = blkno / HASHES_PER_BLOCK;
    uint32_t j = blkno % HASHES_PER_BLOCK;

    oe_assert(blkno < dev->header_block.nblks);
    oe_assert(i < dev->num_hash_blocks);

    if (memcmp(&dev->hash_blocks[i].hashes[j], hash, HASH_SIZE) != 0)
        GOTO(done);

    ret = 0;

done:
    return ret;
}

static int _hash_list_blkdev_release(oefs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        GOTO(done);

    if (oe_atomic_decrement(&dev->ref_count) == 0)
    {
        if (_flush_hash_list(dev) != 0)
            GOTO(done);

        dev->next->release(dev->next);
        free(dev->hash_blocks);
        free(dev->dirty_hash_blocks);
        free(dev);
    }

    ret = 0;

done:
    return ret;
}

static int _hash_list_blkdev_get(
    oefs_blkdev_t* blkdev,
    uint32_t blkno,
    oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    oefs_sha256_t hash;

    if (!dev || !blk || blkno > dev->header_block.nblks)
        GOTO(done);

    if (dev->next->get(dev->next, blkno, blk) != 0)
        GOTO(done);

    if (oefs_sha256(&hash, blk, sizeof(oefs_blk_t)) != 0)
        GOTO(done);

#if defined(TRACE_PUTS_AND_GETS)
    {
        oefs_sha256_str_t str;
        printf("GET{%08u:%.8s}\n", blkno, oefs_sha256_str(&hash, &str));
    }
#endif

    /* Check the hash to make sure the block was not tampered with. */
    if (_check_hash(dev, blkno, &hash) != 0)
    {
        memset(blk, 0, sizeof(oefs_blk_t));
        GOTO(done);
    }

    ret = 0;

done:

    return ret;
}

static int _hash_list_blkdev_put(
    oefs_blkdev_t* blkdev,
    uint32_t blkno,
    const oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    oefs_sha256_t hash;

    oe_assert(blkno < dev->header_block.nblks);

    if (!dev || !blk || blkno >= dev->header_block.nblks)
        GOTO(done);

    if (oefs_sha256(&hash, blk, sizeof(oefs_blk_t)) != 0)
        GOTO(done);

    _set_hash(dev, blkno, &hash);

#if defined(TRACE_PUTS_AND_GETS)
    {
        oefs_sha256_str_t str;
        printf("PUT{%08u:%.8s}\n", blkno, oefs_sha256_str(&hash, &str));
    }
#endif

    if (dev->next->put(dev->next, blkno, blk) != 0)
        GOTO(done);

    ret = 0;

done:

    return ret;
}

static int _hash_list_blkdev_begin(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        GOTO(done);

#if defined(TRACE_PUTS_AND_GETS)
    printf("=== BEGIN\n");
#endif

    if (dev->next->begin(dev->next) != 0)
        GOTO(done);

    ret = 0;

done:

    return ret;
}

static int _hash_list_blkdev_end(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        GOTO(done);

    if (_flush_hash_list(dev) != 0)
        GOTO(done);

#if defined(TRACE_PUTS_AND_GETS)
    printf("=== END\n");
#endif

    if (dev->next->end(dev->next) != 0)
        GOTO(done);

    ret = 0;

done:

    return ret;
}

static int _hash_list_blkdev_add_ref(oefs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        GOTO(done);

    oe_atomic_increment(&dev->ref_count);

    ret = 0;

done:
    return ret;
}

int oefs_hash_list_blkdev_open(
    oefs_blkdev_t** blkdev,
    bool initialize,
    size_t nblks,
    oefs_blkdev_t* next)
{
    int ret = -1;
    blkdev_t* dev = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !next)
        GOTO(done);

    /* There must be at least one block. */
    if (nblks < 1)
        GOTO(done);

    /* Allocate the device structure. */
    if (!(dev = calloc(1, sizeof(blkdev_t))))
        GOTO(done);

    dev->base.get = _hash_list_blkdev_get;
    dev->base.put = _hash_list_blkdev_put;
    dev->base.begin = _hash_list_blkdev_begin;
    dev->base.end = _hash_list_blkdev_end;
    dev->base.add_ref = _hash_list_blkdev_add_ref;
    dev->base.release = _hash_list_blkdev_release;
    dev->ref_count = 1;
    dev->next = next;

    /* Either initialize or load the hash list. */
    if (initialize)
    {
        if (_initialize_hash_list(dev, nblks) != 0)
            GOTO(done);
    }
    else
    {
        if (_load_hash_list(dev) != 0)
            GOTO(done);
    }

    next->add_ref(next);
    *blkdev = &dev->base;
    dev = NULL;

    ret = 0;

done:

    if (dev)
        free(dev);

    return ret;
}

int oefs_hash_list_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks)
{
    int ret = -1;
    size_t hblks;

    if (extra_nblks)
        *extra_nblks = 0;

    if (!extra_nblks)
        GOTO(done);

    hblks = oefs_round_to_multiple(nblks, HASHES_PER_BLOCK) / HASHES_PER_BLOCK;

    *extra_nblks = 1 + hblks;

    ret = 0;

done:
    return ret;
}
