// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "elf.h"

static ve_elf_info_t _elf_info;

uint64_t __ve_self;

void ve_elf_set_info(const ve_elf_info_t* elf_info)
{
    if (elf_info)
        _elf_info = *elf_info;
}

void ve_elf_get_info(ve_elf_info_t* elf_info)
{
    if (elf_info)
        *elf_info = _elf_info;
}

void* ve_elf_get_header(void)
{
    return ((uint8_t*)&__ve_self - _elf_info.self_rva) + _elf_info.base_rva;
}

void* ve_elf_get_baseaddr(void)
{
    return ((uint8_t*)&__ve_self - _elf_info.self_rva);
}
