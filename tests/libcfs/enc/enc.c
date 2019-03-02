// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/limits.h>
#include <openenclave/corelibc/setjmp.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>

oe_jmp_buf __oe_exit_point;
int __oe_exit_status;

int main(int argc, const char* argv[]);

void test_libcfs(const char* src_dir, const char* tmp_dir)
{
    const int argc = 3;
    const char* argv[4] = {
        "main",
        src_dir,
        tmp_dir,
        NULL,
    };

    argv[0] = "./main";
    argv[1] = src_dir;
    argv[2] = tmp_dir;
    argv[3] = NULL;

    __oe_exit_status = OE_INT_MAX;

    if (oe_setjmp(&__oe_exit_point) == 1)
    {
        OE_TEST(__oe_exit_status == 0);
        return;
    }

    main(argc, argv);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
