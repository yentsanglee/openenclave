// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_ELF_H
#define _VE_ENCLAVE_ELF_H

#include "../common/call.h"
#include "common.h"

void ve_elf_set_info(const ve_elf_info_t* elf_info);

void ve_elf_get_info(ve_elf_info_t* elf_info);

/* Get the address of the ELF-64 header. */
void* ve_elf_get_header(void);

/* Get the real base address of this process. */
void* ve_elf_get_baseaddr(void);

#endif /* _VE_ENCLAVE_ELF_H */
