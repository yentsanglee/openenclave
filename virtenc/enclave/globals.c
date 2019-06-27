// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "globals.h"

const void* __ve_shmaddr;
size_t __ve_shmsize;

int g_sock = -1;

size_t g_tdata_rva;
size_t g_tdata_size;
size_t g_tdata_align;
size_t g_tbss_rva;
size_t g_tbss_size;
size_t g_tbss_align;
uint64_t __ve_self;
