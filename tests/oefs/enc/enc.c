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
#include "../../../libc/blockdev.h"
#include "../../../libc/oefs.h"
#include "oefs_t.h"

#define INIT

int test_oefs(const char* oefs_filename)
{
    oe_block_dev_t* dev;
#ifdef INIT
    size_t num_blocks = 4 * 4096;
#endif
    oefs_t* oefs;
    oe_result_t result;
    oefs_result_t r;
    const size_t NUM_FILES = 1000;

    {
        result = oe_open_host_block_dev("/tmp/oefs", &dev);
        OE_TEST(result == OE_OK);
    }

#ifdef INIT
    {
        size_t size;
        r = oefs_compute_size(num_blocks, &size);
        OE_TEST(r == OEFS_OK);
        printf("*** size=%zu\n", size);
    }
#endif

#ifdef INIT
    {
        r = oefs_initialize(dev, num_blocks);
        OE_TEST(r == OEFS_OK);
    }
#endif

    {
        r = oefs_new(&oefs, dev);
        OE_TEST(r == OEFS_OK);
    }

    /* Create multiple files. */
    for (size_t i = 0; i < NUM_FILES; i++)
    {
        char name[OEFS_PATH_MAX];
        snprintf(name, sizeof(name), "filename-%zu", i);

        oefs_file_t* file = NULL;
        r = __oefs_create_file(oefs, OEFS_ROOT_INO, name, &file);
        OE_TEST(r == OEFS_OK);
        OE_TEST(file != NULL);
        oefs_close_file(file);
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
    1,         /* ProductID */
    1,         /* SecurityVersion */
    true,      /* AllowDebug */
    10 * 1024, /* HeapPageCount */
    10 * 1024, /* StackPageCount */
    2);        /* TCSCount */
