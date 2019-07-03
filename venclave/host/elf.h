// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ELF_H
#define _VE_HOST_ELF_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

typedef struct _ve_elf_info
{
    /* Thread local storage .tdata section. */
    size_t tdata_rva;
    size_t tdata_size;
    size_t tdata_align;

    /* Thread local storage .tbss section. */
    size_t tbss_rva;
    size_t tbss_size;
    size_t tbss_align;

    /* The relative virtual address of the first program segment. */
    size_t base_rva;

    /* The relative virtual address of the enclave's __ve_self variable. */
    uint64_t self_rva;
} ve_elf_info_t;

int ve_get_elf_info(const char* path, ve_elf_info_t* buf);

#endif /* _VE_HOST_ELF_H */
