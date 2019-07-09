// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_HOST_ELF_H
#define _VE_HOST_ELF_H

#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>
#include "../common/call.h"

int ve_get_elf_info(const char* path, ve_elf_info_t* buf);

#endif /* _VE_HOST_ELF_H */
