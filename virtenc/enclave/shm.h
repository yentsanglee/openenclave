// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_SHM_H
#define _VE_SHM_H

#include "common.h"

#define VE_SHM_RND 020000
#define VE_SHM_RDONLY 010000
#define VE_SHM_RND 020000
#define VE_SHM_REMAP 040000
#define VE_SHM_EXEC 0100000

/* Same as shmat. */
void* ve_shmat(int shmid, const void* shmaddr, int shmflg);

/* Same as shmdt. */
int ve_shmdt(const void* shmaddr);

#endif /* _VE_SHM_H */
