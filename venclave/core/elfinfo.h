// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_ELFINFO_H
#define _VE_ENCLAVE_ELFINFO_H

#include "../common/call.h"
#include "common.h"

extern ve_elf_info_t __ve_elf_info;

/* Holds relative virtual address of this variable itself (from the host). */
extern uint64_t __ve_self;

#endif /* _VE_ENCLAVE_ELFINFO_H */
