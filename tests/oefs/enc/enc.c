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
#include "../../../libc/buf.h"
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

static void create_files(oefs_t* oefs)
{
    const size_t NUM_FILES = 100;

    for (size_t i = 0; i < NUM_FILES; i++)
    {
        char name[OEFS_PATH_MAX];
        snprintf(name, sizeof(name), "filename-%zu", i);

        oefs_file_t* file = NULL;
        oefs_result_t r = __oefs_create_file(oefs, OEFS_ROOT_INO, name, &file);
        OE_TEST(r == OEFS_OK);
        OE_TEST(file != NULL);
        oefs_close_file(file);
    }
}

static void update_file(oefs_t* oefs, const char* path)
{
    oefs_result_t r;
    oefs_file_t* file = NULL;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    const size_t FILE_SIZE = 1024;
    buf_t buf = BUF_INITIALIZER;
    int32_t n;

    r = oefs_open_file(oefs, path, 0, 0, &file);
    OE_TEST(r == OEFS_OK);

    for (size_t i = 0; i < FILE_SIZE; i++)
    {
        char c = alphabet[i % (sizeof(alphabet)-1)];

        if (buf_append(&buf, &c, 1) != 0)
            OE_TEST(false);
    }

    n = oefs_write_file(file, buf.data, buf.size);
    OE_TEST(n == buf.size);

    oefs_close_file(file);

    /* Load the file to make sure the changes took. */
    {
        void* data;
        size_t size;

        r = oefs_load_file(oefs, path, &data, &size);
        OE_TEST(r == OEFS_OK);
        printf("%.*s\n", (uint32_t)size, data);
        OE_TEST(size == buf.size);
        OE_TEST(memcmp(data, buf.data, size) == 0);

        free(data);
    }

    buf_release(&buf);
}

int test_oefs(const char* oefs_filename)
{
    oe_block_dev_t* dev;
    size_t num_blocks = 4 * 4096;
    oefs_t* oefs;
    oe_result_t result;
    oefs_result_t r;
    size_t size;

    (void)oefs;

    /* Compute the size of the OEFS file. */
    r = oefs_compute_size(num_blocks, &size);
    OE_TEST(r == OEFS_OK);

    /* Ask the host to open the OEFS file. */
    result = oe_open_host_block_dev(oefs_filename, &dev);
    OE_TEST(result == OE_OK);

    /* Initialize the OEFS file. */
    r = oefs_initialize(dev, num_blocks);
    OE_TEST(r == OEFS_OK);

    /* Create an new OEFS object. */
    r = oefs_new(&oefs, dev);
    OE_TEST(r == OEFS_OK);

    /* Create some files. */
    create_files(oefs);

    /* Dump the root directory. */
    dump_dir(oefs, "/");

    /* Test updating of a file. */
    update_file(oefs, "/filename-1");

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
