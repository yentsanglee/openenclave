// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "globals.h"

const void* __ve_shmaddr;
size_t __ve_shmsize;

size_t __ve_tdata_rva;
size_t __ve_tdata_size;
size_t __ve_tdata_align;
size_t __ve_tbss_rva;
size_t __ve_tbss_size;
size_t __ve_tbss_align;

uint64_t __ve_self;

uint64_t __ve_base_rva;

int __ve_main_pid;

/* The socket for the current thread. */
static __thread int _sock = -1;

void ve_set_sock(int sock)
{
    _sock = sock;
}

int ve_get_sock(void)
{
    return _sock;
}
