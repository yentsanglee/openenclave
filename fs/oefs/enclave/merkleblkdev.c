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
#include "sha256.h"
#include "trace.h"
#include "utils.h"

#define HASH_SIZE (sizeof(sha256_t))

#define HASHES_PER_BLOCK (OEFS_BLOCK_SIZE / HASH_SIZE)

#define MAGIC 0xea6a86f99e6a4f83

/* Block layout: [data blocks] [header block] [hash blocks] */

typedef struct _header_block
{
    /* Magic number: MAGIC */
    uint64_t magic;

    /* The total number of data blocks in the system. */
    uint64_t nblks;

    uint8_t reserved[OEFS_BLOCK_SIZE - 16];
} header_block_t;

OE_STATIC_ASSERT(sizeof(header_block_t) == OEFS_BLOCK_SIZE);

typedef struct _hash_block
{
    sha256_t hashes[HASHES_PER_BLOCK];
} hash_block_t;

OE_STATIC_ASSERT(sizeof(hash_block_t) == OEFS_BLOCK_SIZE);

typedef struct _blkdev
{
    oefs_blkdev_t base;
    volatile uint64_t ref_count;
    oefs_blkdev_t* next;

    /*ATTN*/
    header_block_t header_block;

    /* Upper part of Merkle tree (excluding leaf nodes). */
    sha256_t* merkle;

    /* In-memory copy of the hash blocks. */
    hash_block_t* hash_blocks;
    size_t num_hash_blocks;

    /* The dirty hash blocks. */
    uint8_t* dirty_hash_blocks;

    /* True if any dirty_hash_blocks[] elements are non-zero. */
    bool have_dirty_hash_blocks;

} blkdev_t;

OE_INLINE sha256_t* _get_hash(blkdev_t* dev, size_t tree_index)
{
    uint32_t i = tree_index / HASHES_PER_BLOCK;
    uint32_t j = tree_index % HASHES_PER_BLOCK;
    return &dev->hash_blocks[i].hashes[j];
}

OE_INLINE void _set_hash(blkdev_t* dev, size_t tree_index, const sha256_t* hash)
{
    uint32_t i = tree_index / HASHES_PER_BLOCK;
    uint32_t j = tree_index % HASHES_PER_BLOCK;
    dev->hash_blocks[i].hashes[j] = *hash;
    dev->dirty_hash_blocks[i] = 1;
    dev->have_dirty_hash_blocks = true;
}

/* Get the index of the left child of the given node in the hash tree. */
OE_INLINE size_t _left_child_index(size_t i)
{
    return (2 * i) + 1;
}

/* Get the index of the right child of the given node in the hash tree. */
OE_INLINE size_t _right_child_index(size_t i)
{
    return (2 * i) + 2;
}

/* Get the index of the parent of the given node in the hash tree. */
OE_INLINE size_t _parent_index(size_t i)
{
    if (i == 0)
        return -1;

    return (i - 1) / 2;
}

OE_INLINE sha256_t* _left_child(blkdev_t* dev, size_t i)
{
    size_t index = _left_child_index(i);
    size_t merkle_size = dev->header_block.nblks - 1;

    if (index < merkle_size)
        return &dev->merkle[index];
    else
        return _get_hash(dev, index - merkle_size + 1);
}

OE_INLINE sha256_t* _right_child(blkdev_t* dev, size_t i)
{
    size_t index = _right_child_index(i);
    size_t merkle_size = dev->header_block.nblks - 1;

    if (index < merkle_size)
        return &dev->merkle[index];
    else
        return _get_hash(dev, index - merkle_size + 1);
}

static void _hash2(sha256_t* hash, const sha256_t* left, const sha256_t* right)
{
    typedef struct _data
    {
        sha256_t left;
        sha256_t right;
    } data_t;
    data_t data;
    OE_STATIC_ASSERT(sizeof(data) == 64);

    sha256_copy(&data.left, left);
    sha256_copy(&data.right, right);
    sha256_64(hash, &data);
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

static int _flush_merkle(blkdev_t* dev)
{
    int ret = -1;
    size_t blkno;

    if (dev->have_dirty_hash_blocks)
    {
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
    }

    ret = 0;

done:
    return ret;
}

static int _initialize_hash_blocks(blkdev_t* dev)
{
    int ret = -1;
    oefs_blk_t zero_blk;
    sha256_t hash;

    memset(&zero_blk, 0, sizeof(oefs_blk_t));

    if (sha256(&hash, &zero_blk, sizeof(zero_blk)) != 0)
        GOTO(done);

    for (size_t i = 1; i < dev->header_block.nblks + 1; i++)
        _set_hash(dev, i, &hash);

    ret = 0;

done:
    return ret;
}

static void _compute_upper_hash_tree(blkdev_t* dev)
{
    size_t merkle_size = dev->header_block.nblks - 1;

    /* Initialize the non-leaf nodes in reverse. */
    for (size_t i = 0; i < merkle_size; i++)
    {
        size_t index = merkle_size - i - 1;
        sha256_t* left = _left_child(dev, index);
        sha256_t* right = _right_child(dev, index);
        sha256_t hash;

        _hash2(&hash, left, right);
        dev->merkle[index] = hash;
    }
}

static int _load_merkle(blkdev_t* dev)
{
    int ret = -1;
    size_t blkno;
    sha256_t* merkle = NULL;
    hash_block_t* hash_blocks = NULL;
    uint8_t* dirty_hash_blocks = NULL;

    /* Load the header block. */
    {
        if (_load_header_block(dev) != 0)
            GOTO(done);

        if (dev->header_block.magic != MAGIC)
            GOTO(done);
    }

    /* Allocate the merkle[] array. */
    {
        size_t merkle_size = dev->header_block.nblks - 1;
        size_t alloc_size = merkle_size * sizeof(sha256_t);

        if (!(merkle = malloc(alloc_size)))
            GOTO(done);

        dev->merkle = merkle;
    }

    /* Calculate dev->num_hash_blocks */
    dev->num_hash_blocks =
        oefs_round_to_multiple(dev->header_block.nblks + 1, HASHES_PER_BLOCK) /
        HASHES_PER_BLOCK;

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

    /* Compute the hash tree. */
    _compute_upper_hash_tree(dev);

    /* Fail if the computed root hash is wrong. */
    if (!sha256_eq(_get_hash(dev, 0), &dev->merkle[0]))
        GOTO(done);

    merkle = NULL;
    hash_blocks = NULL;
    dirty_hash_blocks = NULL;

    ret = 0;

done:

    if (merkle)
        free(merkle);

    if (hash_blocks)
        free(hash_blocks);

    if (dirty_hash_blocks)
        free(dirty_hash_blocks);

    return ret;
}

static int _init_merkle(blkdev_t* dev, size_t nblks)
{
    int ret = -1;
    sha256_t* merkle = NULL;
    hash_block_t* hash_blocks = NULL;
    uint8_t* dirty_hash_blocks = NULL;

    /* Initialize the header block. */
    {
        memset(&dev->header_block, 0, sizeof(dev->header_block));
        dev->header_block.magic = MAGIC;
        dev->header_block.nblks = nblks;
    }

    /* Allocate the merkle[] array. */
    {
        size_t merkle_size = dev->header_block.nblks - 1;
        size_t alloc_size = merkle_size * sizeof(sha256_t);

        if (!(merkle = malloc(alloc_size)))
            GOTO(done);

        dev->merkle = merkle;
    }

    /* Calculate dev->num_hash_blocks */
    dev->num_hash_blocks =
        oefs_round_to_multiple(dev->header_block.nblks + 1, HASHES_PER_BLOCK) /
        HASHES_PER_BLOCK;

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

        /* Set all hash blocks to dirty. */
        memset(dev->dirty_hash_blocks, 1, alloc_size);
    }

    /* Initialize the hash blocks. */
    if (_initialize_hash_blocks(dev) != 0)
        GOTO(done);

    /* Compute the upper hash tree. */
    _compute_upper_hash_tree(dev);

    /* Update the master hash in the hash block. */
    _set_hash(dev, 0, &dev->merkle[0]);

    /* Flush the header block. */
    if (_flush_header_block(dev) != 0)
        GOTO(done);

    /* Flush the hash blocks to disk. */
    if (_flush_merkle(dev) != 0)
        GOTO(done);

    merkle = NULL;
    hash_blocks = NULL;
    dirty_hash_blocks = NULL;

    ret = 0;

done:

    if (merkle)
        free(merkle);

    if (hash_blocks)
        free(hash_blocks);

    if (dirty_hash_blocks)
        free(dirty_hash_blocks);

    return ret;
}

OE_INLINE int _check_hash(blkdev_t* dev, uint32_t blkno, const sha256_t* hash)
{
    return sha256_eq(_get_hash(dev, blkno + 1), hash) ? 0 : -1;
}

static void _update_hash_tree(
    blkdev_t* dev,
    uint32_t blkno,
    const sha256_t* hash)
{
    size_t merkle_size = dev->header_block.nblks - 1;
    size_t index = merkle_size + blkno;
    size_t parent;

    /* Update the leaf hash. */
    _set_hash(dev, blkno + 1, hash);

    /* Get the index of the parent node. */
    parent = _parent_index(index);

    /* Update hashes of the parent nodes. */
    while (parent != -1)
    {
        sha256_t* left = _left_child(dev, parent);
        sha256_t* right = _right_child(dev, parent);
        _hash2(&dev->merkle[parent], left, right);
        parent = _parent_index(parent);
    }

    /* Update the root hash in the hash block. */
    _set_hash(dev, 0, &dev->merkle[0]);
}

static int _merkle_blkdev_release(oefs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        GOTO(done);

    if (oe_atomic_decrement(&dev->ref_count) == 0)
    {
        if (_flush_merkle(dev) != 0)
            GOTO(done);

        dev->next->release(dev->next);
        free(dev->merkle);
        free(dev->hash_blocks);
        free(dev->dirty_hash_blocks);
        free(dev);
    }

    ret = 0;

done:
    return ret;
}

static int _merkle_blkdev_get(
    oefs_blkdev_t* blkdev,
    uint32_t blkno,
    oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    sha256_t hash;

    if (!dev || !blk || blkno > dev->header_block.nblks)
        GOTO(done);

    if (dev->next->get(dev->next, blkno, blk) != 0)
        GOTO(done);

    if (sha256(&hash, blk, sizeof(oefs_blk_t)) != 0)
        GOTO(done);

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

static int _merkle_blkdev_put(
    oefs_blkdev_t* blkdev,
    uint32_t blkno,
    const oefs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    sha256_t hash;

    oe_assert(blkno < dev->header_block.nblks);

    if (!dev || !blk || blkno >= dev->header_block.nblks)
        GOTO(done);

    if (sha256(&hash, blk, sizeof(oefs_blk_t)) != 0)
        GOTO(done);

    _update_hash_tree(dev, blkno, &hash);

    if (dev->next->put(dev->next, blkno, blk) != 0)
        GOTO(done);

    ret = 0;

done:

    return ret;
}

static int _merkle_blkdev_begin(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        GOTO(done);

    if (dev->next->begin(dev->next) != 0)
        GOTO(done);

    ret = 0;

done:

    return ret;
}

static int _merkle_blkdev_end(oefs_blkdev_t* d)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)d;

    if (!dev || !dev->next)
        GOTO(done);

    if (_flush_merkle(dev) != 0)
        GOTO(done);

    if (dev->next->end(dev->next) != 0)
        GOTO(done);

    ret = 0;

done:

    return ret;
}

static int _merkle_blkdev_add_ref(oefs_blkdev_t* blkdev)
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

int oefs_merkle_blkdev_open(
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

    /* nblks must be greater than 1 and a power of 2. */
    if (!(nblks > 1 && oefs_is_pow_of_2(nblks)))
        GOTO(done);

    /* Allocate the device structure. */
    if (!(dev = calloc(1, sizeof(blkdev_t))))
        GOTO(done);

    dev->base.get = _merkle_blkdev_get;
    dev->base.put = _merkle_blkdev_put;
    dev->base.begin = _merkle_blkdev_begin;
    dev->base.end = _merkle_blkdev_end;
    dev->base.add_ref = _merkle_blkdev_add_ref;
    dev->base.release = _merkle_blkdev_release;
    dev->ref_count = 1;
    dev->next = next;

    /* Either initialize or load the hash list. */
    if (initialize)
    {
        if (_init_merkle(dev, nblks) != 0)
            GOTO(done);
    }
    else
    {
        if (_load_merkle(dev) != 0)
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

int oefs_merkle_blkdev_get_extra_blocks(size_t nblks, size_t* extra_nblks)
{
    int ret = -1;
    size_t num_hash_blocks;

    if (extra_nblks)
        *extra_nblks = 0;

    if (!extra_nblks)
        GOTO(done);

    num_hash_blocks = oefs_round_to_multiple(
        nblks + 1, HASHES_PER_BLOCK) / HASHES_PER_BLOCK;

    *extra_nblks = 1 + num_hash_blocks;

    ret = 0;

done:
    return ret;
}
