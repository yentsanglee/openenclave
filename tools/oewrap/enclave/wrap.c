// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <stdio.h>
#include <sys/mount.h>
#include "wrap_t.h"

extern int main(int argc, const char* argv[]);

extern bool oe_disable_debug_malloc_check;

int oe_wrap_main_ecall(int argc, const void* argv_buf, size_t argv_buf_size)
{
    char** argv = (char**)argv_buf;
    int ret;

    (void)argv_buf_size;

    oe_disable_debug_malloc_check = true;

    /* Translate offsets to pointers. */
    for (int i = 0; i < argc; i++)
        argv[i] = (char*)((uint64_t)argv_buf + (uint64_t)argv[i]);

    if (oe_load_module_host_file_system() != OE_OK)
    {
        fprintf(stderr, "%s: cannot load hostfs()\n", argv[0]);
        return 1;
    }

    if (oe_load_module_host_socket_interface() != OE_OK)
    {
        fprintf(stderr, "%s: cannot load hostsocket()\n", argv[0]);
        return 1;
    }

    if (oe_load_module_host_resolver() != OE_OK)
    {
        fprintf(stderr, "%s: cannot load resolver()\n", argv[0]);
        return 1;
    }

    if (mount("/", "/", OE_HOST_FILE_SYSTEM, 0, NULL) != 0)
    {
        fprintf(stderr, "%s: mount() \n", argv[0]);
        return 1;
    }

    ret = main(argc, (const char**)argv);

    if (umount("/") != 0)
    {
        fprintf(stderr, "%s: umount() \n", argv[0]);
        return 1;
    }

    return ret;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    256,  /* StackPageCount */
    8);   /* TCSCount */
