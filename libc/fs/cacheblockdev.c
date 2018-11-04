// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <assert.h>
#include "blockdev.h"

#define TABLE_SIZE 1093
#define MAX_ENTRIES 64

typedef struct _entry entry_t;

struct _entry
{
    uint32_t blkno;
    uint32_t block[FS_BLOCK_SIZE];
    uint32_t index;
    entry_t* prev;
    entry_t* next;
};

typedef struct _block_dev
{
    fs_block_dev_t base;
    size_t ref_count;
    pthread_spinlock_t lock;
    fs_block_dev_t* next;

    entry_t* table[TABLE_SIZE];
    entry_t* head;
    entry_t* tail;
    size_t count;

} block_dev_t;

static entry_t* _new_entry(
    block_dev_t* dev, uint32_t blkno, const uint8_t block[FS_BLOCK_SIZE])
{
    entry_t* entry;

    if (!(entry = calloc(1, sizeof(entry_t))))
        return NULL;

    entry->blkno = blkno;
    memcpy(entry->block, block, FS_BLOCK_SIZE);

    return entry;
}

static void _free_entry(block_dev_t* dev, entry_t* entry)
{
    free(entry);
}

static void _release_entries(block_dev_t* dev)
{
    for (entry_t* p = dev->head; p; )
    {
        entry_t* next = p->next;
        free(p);
        p = next;
    }
}

static void _remove_entry(block_dev_t* dev, entry_t* entry)
{
    if (entry->prev)
        entry->prev->next = entry->next;
    else
        dev->head = entry->next;

    if (entry->next)
        entry->next->prev = entry->prev;
    else
        dev->tail = entry->prev;

    dev->count--;
}

/* Insert entry at front of the list. */
static void _insert_entry(block_dev_t* dev, entry_t* entry)
{
    if (dev->head)
    {
        entry->prev = NULL;
        entry->next = dev->head;
        dev->head->prev = entry;
        dev->head = entry;
    }
    else
    {
        entry->next = NULL;
        entry->prev = NULL;
        dev->head = entry;
        dev->tail = entry;
    }

    dev->count++;
}

static entry_t* _get_entry(block_dev_t* dev, uint32_t blkno)
{
    size_t h = blkno % TABLE_SIZE;
    size_t n = SIZE_MAX;

    for (size_t i = h; i < n && dev->table[i]; i++)
    {
        if (dev->table[i]->blkno == blkno)
            return dev->table[i];

        /* If this was the last entry in the table, then wrap around. */
        if (i + 1 == TABLE_SIZE)
        {
            i = 0;
            n = h;
        }
    }

    return NULL;
}

static void _put_entry(block_dev_t* dev, entry_t* entry)
{
    size_t h = entry->blkno % TABLE_SIZE;
    bool found_slot = false;

    /* If reached the maximum entry count, evict oldest entry. */
    if (dev->count == MAX_ENTRIES)
    {
        entry_t* p = dev->tail;
        assert(p);
        _remove_entry(dev, p);
        dev->table[p->index] = NULL;
        _free_entry(dev, p);
    }

    /* Insert entry at the front of the list. */
    _insert_entry(dev, entry);

    /* Insert entry into the table. */
    for (size_t i = h, n = SIZE_MAX; i < n; i++)
    {
        if (dev->table[i] == NULL)
        {
            dev->table[i] = entry;
            entry->index = i;
            found_slot = true;
            break;
        }

        /* If this was the last entry in the table, then wrap around. */
        if (i + 1 == TABLE_SIZE)
        {
            i = 0;
            n = h;
        }
    }

    assert(found_slot);
}

/* Move entry to the front of the list. */
static void _touch_entry(block_dev_t* dev, entry_t* entry)
{
    _remove_entry(dev, entry);
    _insert_entry(dev, entry);
}

static int _block_dev_release(fs_block_dev_t* d)
{
    int ret = -1;
    block_dev_t* dev = (block_dev_t*)d;
    size_t new_ref_count;

    if (!dev)
        goto done;

    pthread_spin_lock(&dev->lock);
    new_ref_count = --dev->ref_count;
    pthread_spin_unlock(&dev->lock);

    if (new_ref_count == 0)
    {
        dev->next->release(dev->next);
        _release_entries(dev);
        free(dev);
    }

    ret = 0;

done:
    return ret;
}

static int _block_dev_get(fs_block_dev_t* d, uint32_t blkno, void* data)
{
    int ret = -1;
    block_dev_t* dev = (block_dev_t*)d;
    entry_t* entry;

    if (!dev || !data)
        goto done;

    if ((entry = _get_entry(dev, blkno)))
    {
        memcpy(data, entry->block, FS_BLOCK_SIZE);
        _touch_entry(dev, entry);
    }
    else
    {
        if (dev->next->get(dev->next, blkno, data) != 0)
            goto done;

        if (!(entry = _new_entry(dev, blkno, data)))
            goto done;

        _put_entry(dev, entry);
    }

    ret = 0;

done:

    return ret;
}

static int _block_dev_put(fs_block_dev_t* d, uint32_t blkno, const void* data)
{
    int ret = -1;
    block_dev_t* dev = (block_dev_t*)d;
    entry_t* entry;

    if (!dev || !data)
        goto done;

    if (dev->next->put(dev->next, blkno, data) != 0)
        goto done;

    if ((entry = _get_entry(dev, blkno)))
    {
        memcpy(entry->block, data, FS_BLOCK_SIZE);
        _touch_entry(dev, entry);
    }
    else
    {
        if (!(entry = _new_entry(dev, blkno, data)))
            goto done;

        _put_entry(dev, entry);
    }

    ret = 0;

done:

    return ret;
}

static int _block_dev_add_ref(fs_block_dev_t* d)
{
    int ret = -1;
    block_dev_t* dev = (block_dev_t*)d;

    if (!dev)
        goto done;

    pthread_spin_lock(&dev->lock);
    dev->ref_count++;
    pthread_spin_unlock(&dev->lock);

    ret = 0;

done:
    return ret;
}

int oe_open_cache_block_dev(fs_block_dev_t** dev_out, fs_block_dev_t* next)
{
    int ret = -1;
    block_dev_t* dev = NULL;

    if (dev_out)
        *dev_out = NULL;

    if (!dev_out || !next)
        goto done;

    if (!(dev = calloc(1, sizeof(block_dev_t))))
        goto done;

    dev->base.get = _block_dev_get;
    dev->base.put = _block_dev_put;
    dev->base.add_ref = _block_dev_add_ref;
    dev->base.release = _block_dev_release;
    dev->ref_count = 1;
    dev->next = next;
    next->add_ref(next);

    *dev_out = &dev->base;
    dev = NULL;

    ret = 0;

done:

    if (dev)
        free(dev);

    return ret;
}
