// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "heap.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "hostmalloc.h"
#include "io.h"
#include "trace.h"

ve_heap_t __ve_heap;

/* Create a shared-memory heap for making ecalls and ocalls. */
int ve_heap_create(size_t heap_size)
{
    int ret = -1;
    const int PERM = (S_IRUSR | S_IWUSR);
    int shmid = -1;
    void* shmaddr = NULL;

    if ((shmid = shmget(IPC_PRIVATE, heap_size, PERM)) == -1)
        goto done;

    if ((shmaddr = shmat(shmid, NULL, 0)) == (void*)-1)
        goto done;

    __ve_heap.shmid = shmid;
    __ve_heap.shmaddr = shmaddr;
    __ve_heap.shmsize = heap_size;

    shmid = -1;
    shmaddr = NULL;

    ret = 0;

done:

    if (shmaddr)
        shmdt(shmaddr);

    if (shmid != -1)
        shmctl(shmid, IPC_RMID, NULL);

    return ret;
}
