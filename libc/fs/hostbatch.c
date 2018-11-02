#include "hostbatch.h"
#include <pthread.h>
#include <stdlib.h>
#include <openenclave/enclave.h>

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
    uint8_t* data;
    size_t offset;
    size_t capacity;
    host_block_t* blocks;
};

struct _fs_host_batch
{
    size_t capacity;
    pthread_key_t key;
    thread_data_t* tds;
    pthread_spinlock_t lock;
};

thread_data_t* _new_thread_data(fs_host_batch_t* batch)
{
    thread_data_t* ret = NULL;
    thread_data_t* td = NULL;

    if (!(td = calloc(1, sizeof(thread_data_t))))
        goto done;

    if (!(td->data = oe_host_calloc(1, batch->capacity)))
        goto done;

    td->offset = 0;
    td->capacity = batch->capacity;

    if (pthread_setspecific(batch->key, td) != 0)
        goto done;

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

void _delete_thread_data(thread_data_t* td)
{
    oe_host_free(td->data);

    /* Free the host blocks. */
    for (host_block_t* p = td->blocks; p; )
    {
        host_block_t* next = p->next;
        oe_host_free(p);
        p = next;
    }

    free(td);
}

fs_host_batch_t* fs_host_batch_new(size_t capacity)
{
    fs_host_batch_t* ret = NULL;
    fs_host_batch_t* batch = NULL;

    if (capacity == 0)
        goto done;

    if (!(batch = calloc(1, sizeof(fs_host_batch_t))))
        goto done;

    batch->capacity = capacity;

    if (pthread_key_create(&batch->key, NULL) != 0)
        goto done;

    ret = batch;
    batch = NULL;

done:

    if (batch)
        free(batch);

    return ret;
}

void fs_host_batch_delete(fs_host_batch_t* batch)
{
    if (batch)
    {
        pthread_key_delete(batch->key);

        for (thread_data_t* p = batch->tds; p; )
        {
            thread_data_t* next = p->next;
            _delete_thread_data(p);
            p = next;
        }

        free(batch);
    }
}

void* fs_host_batch_alloc(fs_host_batch_t* batch, size_t size)
{
    void* ret = NULL;
    thread_data_t* td;
    void* ptr = NULL;

    if (!batch)
        goto done;

    if (!(td = pthread_getspecific(batch->key)))
    {
        if (!(td = _new_thread_data(batch)))
            goto done;
    }

    /* Round up to the nearest alignment size. */
    size = (size + ALIGNMENT - 1) / ALIGNMENT;

    if (size <= td->capacity - td->offset)
    {
        ptr = td->data + td->offset;
        td->offset += size;
    }
    else 
    {
        host_block_t* block;

        if (!(block = oe_host_calloc(1, sizeof(host_block_t) + size)))
            goto done;

        block->next = td->blocks;
        td->blocks = block;
        ptr = block->data;
    }

    ret = ptr;

done:
    return ret;
}

int fs_host_batch_free(fs_host_batch_t* batch)
{
    int ret = -1;
    thread_data_t* td;

    if (!batch)
        goto done;

    if (!(td = pthread_getspecific(batch->key)))
    {
        if (!(td = _new_thread_data(batch)))
            goto done;
    }

    /* Rewind the data area. */
    td->offset = 0;

    /* Free the host blocks. */
    {
        for (host_block_t* p = td->blocks; p; )
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
