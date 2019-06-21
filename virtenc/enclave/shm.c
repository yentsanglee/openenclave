// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "shm.h"
#include "syscall.h"

void* ve_shmat(int shmid, const void* shmaddr, int shmflg)
{
    return (void*)ve_syscall3(OE_SYS_shmat, shmid, (long)shmaddr, shmflg);
}

int ve_shmdt(const void* shmaddr)
{
    return (int)ve_syscall1(OE_SYS_shmat, (long)shmaddr);
}
