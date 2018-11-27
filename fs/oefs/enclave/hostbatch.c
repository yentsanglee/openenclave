// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hostbatch.h"
#include <assert.h>
#include <openenclave/enclave.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIGNMENT sizeof(uint64_t)

typedef struct _thread_data thread_data_t;
typedef struct _host_block host_block_t;

struct _host_block
{
    host_block_t* next;
    uint8_t data[];
};

struct _thread_data
{
    thread_data_t* next;
    pthread_t thread;
    uint8_t* data;
    size_t offset;
    size_t capacity;
    host_block_t* blocks;
};

#define GUARD 0xa89d0e55

struct _oefs_host_batch
{
    size_t capacity;
    thread_data_t* tds;
    uint32_t guard1;
    pthread_spinlock_t lock;
    uint32_t guard2;
};

static thread_data_t* _new_thread_data(oefs_host_batch_t* batch)
{
    thread_data_t* ret = NULL;
    thread_data_t* td = NULL;

    if (!(td = calloc(1, sizeof(thread_data_t))))
        goto done;

    if (!(td->data = oe_host_calloc(1, batch->capacity)))
        goto done;

    td->thread = pthread_self();
    td->offset = 0;
    td->capacity = batch->capacity;

    /* Add new thread data to the list. */
    pthread_spin_lock(&batch->lock);
    td->next = batch->tds;
    batch->tds = td;
    pthread_spin_unlock(&batch->lock);

    ret = td;
    td = NULL;

done:

    if (td)
        free(td);

    return ret;
}

static thread_data_t* _get_thread_data(oefs_host_batch_t* batch)
{
    thread_data_t* td = NULL;

    /* Find the thread data for the current thread. */
    pthread_spin_lock(&batch->lock);
    {
        for (thread_data_t* p = batch->tds; p; p = p->next)
        {
            if (pthread_equal(p->thread, pthread_self()))
            {
                td = p;
                break;
            }
        }
    }
    pthread_spin_unlock(&batch->lock);

    return td;
}

static void _delete_thread_data(thread_data_t* td)
{
    oe_host_free(td->data);

    /* free the host blocks. */
    for (host_block_t* p = td->blocks; p;)
    {
        host_block_t* next = p->next;
        oe_host_free(p);
        p = next;
    }

    free(td);
}

oefs_host_batch_t* oefs_host_batch_new(size_t capacity)
{
    oefs_host_batch_t* ret = NULL;
    oefs_host_batch_t* batch = NULL;

    if (capacity == 0)
        goto done;

    if (!(batch = calloc(1, sizeof(oefs_host_batch_t))))
        goto done;

    batch->capacity = capacity;
    batch->guard1 = GUARD;
    batch->guard2 = GUARD;

    ret = batch;
    batch = NULL;

done:

    if (batch)
        free(batch);

    return ret;
}

void oefs_host_batch_delete(oefs_host_batch_t* batch)
{
    if (batch)
    {
        for (thread_data_t* p = batch->tds; p;)
        {
            thread_data_t* next = p->next;
            _delete_thread_data(p);
            p = next;
        }

        free(batch);
    }
}

void* oefs_host_batch_malloc(oefs_host_batch_t* batch, size_t size)
{
    void* ret = NULL;
    thread_data_t* td;
    void* ptr = NULL;
    size_t total_size;

    if (!batch)
        goto done;

    if (!(td = _get_thread_data(batch)))
    {
        if (!(td = _new_thread_data(batch)))
            goto done;
    }

    /* Round up to the nearest alignment size. */
    total_size = (size + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT;

    if (total_size <= td->capacity - td->offset)
    {
        ptr = td->data + td->offset;
        td->offset += total_size;
    }
    else
    {
        host_block_t* blk;

        if (!(blk = oe_host_calloc(1, sizeof(host_block_t) + total_size)))
            goto done;

        blk->next = td->blocks;
        td->blocks = blk;
        ptr = blk->data;
    }

    ret = ptr;

done:
    return ret;
}

void* oefs_host_batch_calloc(oefs_host_batch_t* batch, size_t size)
{
    void* ptr;

    if (!(ptr = oefs_host_batch_malloc(batch, size)))
        return NULL;

    return memset(ptr, 0, size);
}

char* oefs_host_batch_strdup(oefs_host_batch_t* batch, const char* str)
{
    char* ret = NULL;
    size_t len;

    if (!batch || !str)
        goto done;

    len = strlen(str);

    if (!(ret = oefs_host_batch_calloc(batch, len + 1)))
        goto done;

    memcpy(ret, str, len + 1);

done:
    return ret;
}

int oefs_host_batch_free(oefs_host_batch_t* batch)
{
    int ret = -1;
    thread_data_t* td;

    if (!batch)
        goto done;

    if (!(td = _get_thread_data(batch)))
    {
        if (!(td = _new_thread_data(batch)))
            goto done;
    }

    /* Rewind the data area. */
    td->offset = 0;

    /* free the host blocks. */
    {
        for (host_block_t* p = td->blocks; p;)
        {
            host_block_t* next = p->next;
            oe_host_free(p);
            p = next;
        }

        td->blocks = NULL;
    }

    ret = 0;

done:
    return ret;
}
