// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include "wrap_t.h"

extern int main(int argc, const char* argv[]);

int oe_wrap_main_ecall(int argc, const void* argv_buf, size_t argv_buf_size)
{
    char** argv = (char**)argv_buf;

    (void)argv_buf_size;

    /* Translate offsets to pointers. */
    for (int i = 0; i < argc; i++)
        argv[i] = (char*)((uint64_t)argv_buf + (uint64_t)argv[i]);

    return main(argc, (const char**)argv);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    256,  /* StackPageCount */
    8);   /* TCSCount */
