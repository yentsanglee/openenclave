// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_SHM_H
#define _VE_ENCLAVE_SHM_H

#include "common.h"

#define VE_SHM_RND 020000
#define VE_SHM_RDONLY 010000
#define VE_SHM_RND 020000
#define VE_SHM_REMAP 040000
#define VE_SHM_EXEC 0100000

/* Implementation of POSIX shmat function. */
void* ve_shmat(int shmid, const void* shmaddr, int shmflg);

/* Implementation of POSIX shmdt function. */
int ve_shmdt(const void* shmaddr);

#endif /* _VE_ENCLAVE_SHM_H */
