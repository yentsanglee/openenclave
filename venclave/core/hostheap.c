// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "hostheap.h"
#include "print.h"
#include "shm.h"

/* Memory shared by the host. */
static void* _start;
static size_t _size;

int ve_host_heap_attach(int shmid, void* shmaddr, size_t shmsize)
{
    int ret = -1;
    void* rval;
    const int shmflg = VE_SHM_RND | VE_SHM_REMAP;

    if (shmid == -1 || shmaddr == NULL || shmaddr == (void*)-1)
        goto done;

    /* Attach the host's shared memory heap. */
    if ((rval = ve_shmat(shmid, shmaddr, shmflg)) == (void*)-1)
    {
        ve_printf(
            "error: ve_shmat(1) failed: rval=%p shmid=%d shmaddr=%p\n",
            rval,
            shmid,
            shmaddr);
        goto done;
    }

    if (rval != shmaddr)
    {
        ve_printf(
            "error: ve_shmat(2) failed: rval=%p shmid=%d shmaddr=%p\n",
            rval,
            shmid,
            shmaddr);
        goto done;
    }

    /* Initialize the host heap. */
    _start = shmaddr;
    _size = shmsize;

    ret = 0;

done:
    return ret;
}

void* ve_host_heap_get_start(void)
{
    return _start;
}

void* ve_host_heap_get_end(void)
{
    return (uint8_t*)_start + _size;
}

size_t ve_host_heap_get_size(void)
{
    return _size;
}
