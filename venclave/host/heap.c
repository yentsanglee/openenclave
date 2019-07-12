// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "heap.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>

/* Create a shared-memory heap for making ecalls and ocalls. */
int ve_heap_create(ve_heap_t* heap, size_t heap_size)
{
    int ret = -1;
    const int PERM = (S_IRUSR | S_IWUSR);
    int shmid = -1;
    void* shmaddr = (void*)-1;

    if (heap)
        memset(heap, 0, sizeof(ve_heap_t));

    if (!heap || heap_size == 0)
        goto done;

    errno = 0;

    if ((shmid = shmget(IPC_PRIVATE, heap_size, PERM)) == -1)
    {
        printf("HEAP_SIZE=%zu\n", heap_size);
        printf("ERRNO=%d\n", errno);
        goto done;
    }

    if ((shmaddr = shmat(shmid, NULL, 0)) == (void*)-1)
        goto done;

    heap->shmid = shmid;
    heap->shmaddr = shmaddr;
    heap->shmsize = heap_size;

    shmid = -1;
    shmaddr = (void*)-1;

    ret = 0;

done:

    if (shmaddr != (void*)-1)
        shmdt(shmaddr);

    if (shmid != -1)
        shmctl(shmid, IPC_RMID, NULL);

    return ret;
}
