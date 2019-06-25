// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ELF_H
#define _VE_HOST_ELF_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

typedef struct _ve_tls_info
{
    size_t tdata_size;
    size_t tdata_align;
    size_t tbss_size;
    size_t tbss_align;
} ve_tls_info_t;

int ve_get_elf_tls_info(const char* path, ve_tls_info_t* buf);

#endif /* _VE_HOST_ELF_H */
