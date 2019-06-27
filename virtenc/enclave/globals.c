// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "globals.h"

const void* __ve_shmaddr;
size_t __ve_shmsize;

int __ve_sock = -1;

size_t __ve_tdata_rva;
size_t __ve_tdata_size;
size_t __ve_tdata_align;
size_t __ve_tbss_rva;
size_t __ve_tbss_size;
size_t __ve_tbss_align;

uint64_t __ve_self;

__thread int __ve_thread_sock_tls;
