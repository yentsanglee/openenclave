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
#include "../../../libc/blockdevice.h"
#include "../../../libc/oefs.h"
#include "oefs_t.h"

#define INIT

int test_oefs(const char* oefs_filename)
{
    oe_block_device_t* dev;
    size_t num_blocks = 10*4096;
    oefs_t* oefs;

    {
        oe_result_t r = oe_open_block_device("/tmp/oefs", &dev);
        OE_TEST(r == OE_OK);
    }

    {
        size_t size;
        oefs_result_t r = oefs_compute_size(num_blocks, &size);
        OE_TEST(r == OEFS_OK);
        printf("*** size=%zu\n", size);
    }

#ifdef INIT
    {
        oefs_result_t r = oefs_initialize(dev, num_blocks);
        OE_TEST(r == OEFS_OK);
    }
#endif

    {
        oefs_result_t r = oefs_new(&oefs, dev);
        OE_TEST(r == OEFS_OK);
    }

    {
        oefs_dir_t* dir;
        oefs_dirent_t* ent;

        dir = oefs_opendir(oefs, "/");
        OE_TEST(dir != NULL);

        while ((ent = oefs_readdir(dir)))
        {
            printf("name=%s\n", ent->d_name);
        }

        oefs_closedir(dir);
    }

    oefs_delete(oefs);
    dev->close(dev);

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    10*1024, /* HeapPageCount */
    10*1024, /* StackPageCount */
    2);   /* TCSCount */
