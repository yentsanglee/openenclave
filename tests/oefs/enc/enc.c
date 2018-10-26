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

#define BLOCK_SIZE 512

static void dump_dir(oefs_t* oefs, const char* dirname)
{
    oefs_dir_t* dir;
    oefs_dirent_t* ent;

    dir = oefs_opendir(oefs, dirname);
    OE_TEST(dir != NULL);

    while ((ent = oefs_readdir(dir)))
    {
        printf("name=%s\n", ent->d_name);
    }

    oefs_closedir(dir);
}

int test_oefs(const char* oefs_filename)
{
    oe_block_dev_t* dev;
    size_t num_blocks = 4 * 4096;
    oefs_t* oefs;
    oe_result_t result;
    oefs_result_t r;
    const size_t NUM_FILES = 100;
    size_t size;

    (void)NUM_FILES;
    (void)oefs;

    {
        r = oefs_compute_size(num_blocks, &size);
        OE_TEST(r == OEFS_OK);
    }

#if 1
    {
        result = oe_open_host_block_dev("/tmp/oefs", &dev);
        OE_TEST(result == OE_OK);
    }
#else
    {
        result = oe_open_ram_block_dev(size, &dev);
        OE_TEST(result == OE_OK);
        OE_TEST(dev != NULL);
    }
#endif


#if 1
#ifdef INIT
    {
        r = oefs_initialize(dev, num_blocks);
        OE_TEST(r == OEFS_OK);
    }
#endif
#endif

    r = oefs_new(&oefs, dev);
    OE_TEST(r == OEFS_OK);

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

    dump_dir(oefs, "/");

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
