// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blkdev.h"
#include "sha.h"
#include "atomic.h"

#define NODES_PER_BLOCK (FS_BLOCK_SIZE / sizeof(node_t))

/* Merkle tree node. Non-leaf nodes have zero, one, or two children. */
typedef struct _node
{
    /* The hash of the (1) left, (2) right, or (3) left and right. */
    fs_sha256_t hash;

    /* The index of a leaf node (or zero for null). */
    uint32_t left;

    /* The index of a leaf node (or zero for null). */
    uint32_t right;
}
node_t;

FS_STATIC_ASSERT(sizeof(node_t) == 40);

typedef struct _blkdev
{
    fs_blkdev_t base;
    volatile uint64_t ref_count;
    fs_blkdev_t* next;
    size_t nblks;
    const node_t* nodes;
    size_t nnodes;

    /* The number of hash blocks. */
    size_t nnodeblks;

    /* Keeps track of dirty hash blocks. */
    uint8_t* dirty;
} blkdev_t;

FS_INLINE bool _is_power_of_two(size_t n)
{
    return (n & (n - 1)) == 0;
}

/* Get the index of the left child of the given node in the Merkle tree. */
FS_INLINE size_t _lchild(size_t i)
{
    return (2 * i) + 1;
}

/* Get the index of the right child of the given node in the Merkle tree. */
FS_INLINE size_t _rchild(size_t i)
{
    return (2 * i) + 2;
}

/* Get the index of the parent of the given node in the Merkle tree. */
FS_INLINE size_t _parent(size_t i)
{
    if (i == 0)
        return -1;

    return (i - 1) / 2;
}

typedef enum _direction
{
    D_NONE,
    D_LEFT = 'L',
    D_RIGHT = 'R',
}
direction_t;

/* Get the direction of node n in relative to root (left, right, or none). */
static direction_t _direction(size_t root, size_t n)
{
    size_t left;
    size_t right;
    size_t i;

    if (n == 0 || root >= n)
        return D_NONE;

    left = _lchild(root);
    right = _rchild(root);
    i = n;

    while (i != 0)
    {
        if (i == left)
            return D_LEFT;

        if (i == right)
            return D_RIGHT;

        i =  _parent(i);
    }

    return D_NONE;
}

FS_INLINE bool _is_null_node(const node_t* node)
{
    return node->left == 0 && node->right == 0;
}

FS_INLINE size_t _round_to_multiple(size_t x, size_t m)
{
    return (size_t)((x + (m - 1)) / m * m);
}

FS_INLINE size_t _min(size_t x, size_t y)
{
    return (x < y) ? x : y;
}

FS_INLINE int _is_leaf(blkdev_t* dev, size_t index)
{
    return index >= (dev->nblks - 1) && index < dev->nnodes;
}

/* Find the hash of two nodes: i and j. */
static int _hash(blkdev_t* dev, fs_sha256_t* hash, uint32_t i, uint32_t j)
{
    int ret = -1;
    fs_vector_t v[2];

    v[0].data = (void*)&dev->nodes[i].hash;
    v[0].size = sizeof(fs_sha256_t);

    v[1].data = (void*)&dev->nodes[j].hash;
    v[1].size = sizeof(fs_sha256_t);

    if (fs_sha256_v(hash, v, FS_COUNTOF(v)) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

static void _set_node(
    blkdev_t* dev, 
    size_t i, 
    const fs_sha256_t* hash,
    uint32_t left,
    uint32_t right)
{
    assert(i < dev->nnodes);

    /* Update the node. */
    {
        node_t* node;
        node = (node_t*)&dev->nodes[i];
        node->hash = *hash;
        node->left = left;
        node->right = right;
    }

    /* Determine which blocks will need to be updated. */
    {
        const uint8_t* start = (const uint8_t*)dev->nodes;
        const uint8_t* p = (const uint8_t*)&dev->nodes[i];
        size_t lo_byte = p - start;
        size_t hi_byte = (p + sizeof(node_t) - 1) - start;

        dev->dirty[lo_byte / FS_BLOCK_SIZE] = 1;
        dev->dirty[hi_byte / FS_BLOCK_SIZE] = 1;
    }
}

/* Write the nodes just after the data blocks. */
static int _write_mtree(blkdev_t* dev)
{
    int ret = -1;
    const fs_blk_t* p = (const fs_blk_t*)dev->nodes;

    for (size_t i = 0; i < dev->nnodeblks; i++)
    {
        if (dev->dirty[i])
        {
            if (dev->next->put(dev->next, i + dev->nblks, p) != 0)
                goto done;

            dev->dirty[i] = 0;
        }

        p++;
    }

    ret = 0;

done:
    return ret;
}

/* Read the hashes just after the data blocks. */
static int read_mtree(blkdev_t* dev)
{
    int ret = -1;
    size_t nbytes = dev->nnodes * sizeof(node_t);
    size_t nblks = _round_to_multiple(nbytes, FS_BLOCK_SIZE) / FS_BLOCK_SIZE;
    uint8_t* ptr = (uint8_t*)dev->nodes;
    size_t rem = dev->nnodes * sizeof(node_t);

    for (size_t i = 0; i < nblks; i++)
    {
        fs_blk_t blk;
        size_t n = _min(rem, sizeof(blk));

        if (dev->next->get(dev->next, i + dev->nblks, &blk) != 0)
            goto done;

        memcpy(ptr, &blk, n);
        ptr += n;
        rem -= n;
    }

    ret = 0;
    goto done;

done:
    return ret;
}

/* Check that the hashes in the Merkle tree are correct. */
static int _check_mtree(blkdev_t* dev)
{
    int ret = -1;

    /* Check the hashes of all non-leaf nodes. */
    for (size_t i = 0; i < dev->nblks - 1; i++)
    {
        const node_t* node = &dev->nodes[i];
        fs_sha256_t buf;
        const fs_sha256_t* hash;

        /* Skip null nodes. */
        if (!node->left && !node->right)
            continue;

        if (node->left && node->right)
        {
            /* Both left and right children are present. */
            if (_hash(dev, &buf, node->left, node->right) != 0)
                goto done;

            hash = &buf;
        }
        else if (node->left)
        {
            /* Only the left child is present. */
            hash = &dev->nodes[node->left].hash;
        }
        else if (node->right)
        {
            /* Only the right child is present. */
            hash = &dev->nodes[node->right].hash;
        }

        if (!fs_sha256_eq(hash, &dev->nodes[i].hash))
            goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _check_hash(blkdev_t* dev, uint32_t blkno, const fs_sha256_t* hash)
{
    int ret = -1;
    size_t index = (dev->nblks - 1) + blkno;

    if (index > dev->nnodes)
        goto done;

    if (!fs_sha256_eq(&dev->nodes[index].hash, hash))
        goto done;

    ret = 0;

done:
    return ret;
}

static int _update_up(blkdev_t* dev, uint32_t root, uint32_t child)
{
    int ret = -1;
    uint32_t left = 0;
    uint32_t right = 0;
    const node_t* node = &dev->nodes[root];
    fs_sha256_t hash;

    assert(child != 0);

    if (child == _lchild(root))
    {
        left = child;
        right = node->right;
    }
    else if (child == _rchild(root))
    {
        left = node->left;
        right = child;
    }
    else
    {
        assert(0);
    }

    if (_hash(dev, &hash, left, right) != 0)
        goto done;

    _set_node(dev, root, &hash, left, right);

    if (root != 0)
    {
        if (_update_up(dev, _parent(root), root) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _update_down(
    blkdev_t* dev, 
    uint32_t root, /* null node to be updated. */
    uint32_t old,  /* old leaf node index. */
    uint32_t new)  /* new leaf node index. */
{
    int ret = -1;
    direction_t d_old = _direction(root, old);
    direction_t d_new = _direction(root, new);

    assert(root < dev->nnodes);

    /* Terminate when a leaf node is reached. */
    if (_is_leaf(dev, root))
    {
        ret = 0;
        goto done;
    }

    if (root >= dev->nnodes || d_old == D_NONE || d_new == D_NONE)
        goto done;
    
    if (d_old == D_LEFT && d_new == D_LEFT) /* two left shoes */
    {
        if (_update_down(dev, _lchild(root), old, new) != 0)
            goto done;
    }
    else if (d_old == D_RIGHT && d_new == D_RIGHT) /* two right shoes */
    {
        if (_update_down(dev, _rchild(root), old, new) != 0)
            goto done;
    }
    else /* one left shoe and one right shoe */
    {
        uint32_t left = (d_old == D_LEFT) ? d_old : d_new;
        uint32_t right = (d_old == D_RIGHT) ? d_old : d_new;
        fs_sha256_t h;

        if (_hash(dev, &h, left, right) != 0)
            goto done;

        _set_node(dev, root, &h, left, right);

        if (_update_up(dev, _parent(root), root) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

static int _update_mtree(
    blkdev_t* dev,
    uint32_t blkno,
    const fs_sha256_t* hash)
{
    int ret = -1;
    size_t leaf = (dev->nblks - 1) + blkno;
    size_t ancestor;
    direction_t d;
    fs_sha256_t h;

    assert(_is_leaf(dev, leaf));

    if (!_is_leaf(dev, leaf))
        goto done;

    /* Update the leaf node. */
    _set_node(dev, leaf, hash, 0, 0);

    /* Find the first non-null ancestor. */
    {
        ancestor = _parent(leaf);

        while (ancestor != -1 && _is_null_node(&dev->nodes[ancestor]))
        {
            ancestor = _parent(ancestor);
        }
    }

    if (ancestor == -1) /* empty root */
    {
        uint32_t root = 0;

        if ((d = _direction(root, leaf)) == D_NONE)
            goto done;

        if (d == D_LEFT)
            _set_node(dev, root, hash, leaf, 0);
        else
            _set_node(dev, root, hash, 0, leaf);
    }
    else /* non-empty ancestor */
    {
        const node_t* node = &dev->nodes[ancestor];

        if ((d = _direction(ancestor, leaf)) == D_NONE)
            goto done;

        if (d == D_LEFT)
        {
            if (node->left && node->left != leaf)
            {
                uint32_t old = node->left;
                uint32_t new = leaf;

                if (_update_down(dev, _lchild(ancestor), old, new) != 0)
                {
                    goto done;
                }
            }
            else
            {
                fs_sha256_t h;

                if (_hash(dev, &h, leaf, node->right) != 0)
                    goto done;

                _set_node(dev, ancestor, &h, leaf, node->right);
            }
        }
        else
        {
            if (node->right && node->right != leaf)
            {
                uint32_t old = node->right;
                uint32_t new = leaf;

                if (_update_down(dev, _rchild(ancestor), old, new) != 0)
                    goto done;
            }
            else
            {
                if (_hash(dev, &h, node->left, leaf) != 0)
                    goto done;

                _set_node(dev, ancestor, &h, node->left, leaf);
            }
        }
    }

    ret = 0;

done:
    return ret;
}

static int _blkdev_release(fs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        goto done;

    if (fs_atomic_decrement(&dev->ref_count) == 0)
    {
        dev->next->release(dev->next);
        free((void*)dev->nodes);
        free(dev->dirty);
        free(dev);
    }

    ret = 0;

done:
    return ret;
}

static int _blkdev_get(fs_blkdev_t* blkdev, uint32_t blkno, fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    fs_sha256_t hash;

    if (!dev || !blk || blkno > dev->nblks)
        goto done;

    if (dev->next->get(dev->next, blkno, blk) != 0)
        goto done;

    if (fs_sha256(&hash, blk, sizeof(fs_blk_t)) != 0)
        goto done;

    /* Check the hash to make sure the block was not tampered with. */
    if (_check_hash(dev, blkno, &hash) != 0)
    {
        memset(blk, 0, sizeof(fs_blk_t));
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static int _blkdev_put(fs_blkdev_t* blkdev, uint32_t blkno, const fs_blk_t* blk)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;
    fs_sha256_t hash;

    if (!dev || !blk || blkno > dev->nblks)
        goto done;

    if (fs_sha256(&hash, blk, sizeof(fs_blk_t)) != 0)
        goto done;

    if (_update_mtree(dev, blkno, &hash) != 0)
        goto done;

    if (dev->next->put(dev->next, blkno, blk) != 0)
        goto done;

#if defined(EXTRA_CHECKS)
    if (_check_mtree(dev) != 0)
        goto done;
#endif

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

    if (_write_mtree(dev) != 0)
        goto done;

    if (dev->next->end(dev->next) != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

static int _blkdev_add_ref(fs_blkdev_t* blkdev)
{
    int ret = -1;
    blkdev_t* dev = (blkdev_t*)blkdev;

    if (!dev)
        goto done;

    fs_atomic_increment(&dev->ref_count);

    ret = 0;

done:
    return ret;
}

int fs_open_merkle_blkdev(
    fs_blkdev_t** blkdev,
    size_t nblks,
    bool initialize,
    fs_blkdev_t* next)
{
    int ret = -1;
    blkdev_t* dev = NULL;
    node_t* nodes = NULL;
    size_t nnodes;
    uint8_t* dirty = NULL;

    if (blkdev)
        *blkdev = NULL;

    if (!blkdev || !next)
        goto done;

    /* nblks must be greater than 1 and a power of 2. */
    if (!(nblks > 1 && _is_power_of_two(nblks)))
        goto done;

    /* Calculate the number of nodes in the Merkle tree. */
    nnodes = (nblks * 2) - 1;

    /* Allocate the device structure. */
    if (!(dev = calloc(1, sizeof(blkdev_t))))
        goto done;

    /* Allocate the Merkle tree nodes (allocate multiple of the block size). */
    {
        size_t size = nnodes * sizeof(node_t);

        size = _round_to_multiple(size, FS_BLOCK_SIZE);

        if (!(nodes = calloc(1, size)))
            goto done;

        dev->nnodeblks = size / FS_BLOCK_SIZE;
    }

    /* Allocate the dirty bytes for the Merkle tree node blocks. */
    if (!(dirty = calloc(1, dev->nnodeblks)))
        goto done;

    dev->base.get = _blkdev_get;
    dev->base.put = _blkdev_put;
    dev->base.begin = _blkdev_begin;
    dev->base.end = _blkdev_end;
    dev->base.add_ref = _blkdev_add_ref;
    dev->base.release = _blkdev_release;
    dev->ref_count = 1;
    dev->next = next;
    dev->nblks = nblks;
    dev->nodes = nodes;
    dev->nnodes = nnodes;
    dev->dirty = dirty;

    if (initialize)
    {
        /* Set all the dirty bits so all nodes will be written. */
        memset(dev->dirty, 1, dev->nnodeblks);

        if (_check_mtree(dev) != 0)
            goto done;

        /* Write the Merkle tree (contains all null nodes initially). */
        if (_write_mtree(dev) != 0)
            goto done;
    }
    else
    {
        if (read_mtree(dev) != 0)
            goto done;

        if (_check_mtree(dev) != 0)
            goto done;
    }

    {
        printf("blocksize=%zu\n", FS_BLOCK_SIZE);
        printf("nblks=%zu\n", dev->nblks);
        printf("extra=%zu\n", dev->nnodeblks);
    }

    next->add_ref(next);
    *blkdev = &dev->base;
    nodes = NULL;
    dirty = NULL;
    dev = NULL;

    ret = 0;

done:

    if (dev)
        free(dev);

    if (nodes)
        free(nodes);

    if (dirty)
        free(dirty);

    return ret;
}
