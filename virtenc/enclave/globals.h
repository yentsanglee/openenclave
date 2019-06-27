// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_GLOBALS_H
#define _VE_ENCLAVE_GLOBALS_H

#include "common.h"

/* Memory shared by the host. */
extern const void* __ve_shmaddr;
extern size_t __ve_shmsize;

/* Socket used to communicate with the host process. */
extern int __ve_sock;

/* The .tdata section of this process (sent from the host). */
extern size_t __ve_tdata_rva;
extern size_t __ve_tdata_size;
extern size_t __ve_tdata_align;

/* The .tbss section of this process (sent from the host). */
extern size_t __ve_tbss_rva;
extern size_t __ve_tbss_size;
extern size_t __ve_tbss_align;

/* Holds relative virtual address of this variable itself (from the host). */
extern uint64_t __ve_self;

#endif /* _VE_ENCLAVE_GLOBALS_H */
