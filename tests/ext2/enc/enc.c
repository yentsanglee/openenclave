// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <limits.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/file.h>
#include <openenclave/internal/mount.h>
#include <openenclave/internal/tests.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ext2_t.h"

int test_ext2(const char* ext2_filename)
{
    oe_result_t r;
    oe_file_t* file = NULL;

    r = oe_mount(OE_MOUNT_TYPE_EXT2, ext2_filename, "/mnt/test_ext2");
    OE_TEST(r == OE_OK);

    /* Open the file */
    file = oe_file_open("/mnt/test_ext2/a/b/c/Makefile", 0, 0);
    OE_TEST(file != NULL);

    /* Read the file. */
    {
        char buf[16];
        ssize_t n;
        ssize_t total = 0;

        while ((n = oe_file_read(file, buf, sizeof(buf))) > 0)
        {
            printf("%.*s", (unsigned int)n, buf);
            total += n;
        }

        printf("\n");

        OE_TEST(total != 0);
    }

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
