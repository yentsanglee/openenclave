// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <ctype.h>
#include <limits.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/file.h>
#include <openenclave/internal/mount.h>
#include <openenclave/internal/tests.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../../../libc/blockdev.h"
#include "../../../libc/buf.h"
#include "../../../libc/oefs.h"
#include "oefs_t.h"

#define INIT

#define BLOCK_SIZE 512

static void _dump_dir(oefs_t* oefs, const char* dirname)
{
    oefs_dir_t* dir;
    oefs_dirent_t* ent;

    dir = oefs_opendir(oefs, dirname);
    OE_TEST(dir != NULL);

    printf("<<<<<<<< _dump_dir(%s)\n", dirname);

    while ((ent = oefs_readdir(dir)))
    {
        printf("name=%s\n", ent->d_name);
    }

    printf(">>>>>>>>\n\n");

    oefs_closedir(dir);
}

static void _create_files(oefs_t* oefs)
{
    const size_t NUM_FILES = 100;
    oefs_result_t r;
    oefs_file_t* file;

    for (size_t i = 0; i < NUM_FILES; i++)
    {
        char path[OEFS_PATH_MAX];
        snprintf(path, sizeof(path), "/filename-%04zu", i);
        r = oefs_create(oefs, path, 0, &file);
        OE_TEST(r == OEFS_OK);
        OE_TEST(file != NULL);
        oefs_close(file);
    }
}

static void _remove_files_nnnn(oefs_t* oefs)
{
    const size_t NUM_FILES = 100;
    oefs_result_t r;

    for (size_t i = 0; i < NUM_FILES; i++)
    {
        char path[OEFS_PATH_MAX];
        snprintf(path, sizeof(path), "/filename-%04zu", i);
        r = oefs_unlink(oefs, path);
        OE_TEST(r == OEFS_OK);
    }
}

static void _create_dirs(oefs_t* oefs)
{
    const size_t NUM_DIRS = 10;
    oefs_result_t r;

    for (size_t i = 0; i < NUM_DIRS; i++)
    {
        char name[OEFS_PATH_MAX];
        snprintf(name, sizeof(name), "/dir-%04zu", i);

        r = oefs_mkdir(oefs, name, 0);
        OE_TEST(r == OEFS_OK);
    }

    r = oefs_mkdir(oefs, "/aaa", 0);
    OE_TEST(r == OEFS_OK);

    r = oefs_mkdir(oefs, "/aaa/bbb", 0);
    OE_TEST(r == OEFS_OK);

    r = oefs_mkdir(oefs, "/aaa/bbb/ccc", 0);
    OE_TEST(r == OEFS_OK);

    /* Stat root directory and check expectations. */
    {
        oefs_stat_t stat;
        r = oefs_stat(oefs, "/", &stat);
        OE_TEST(r == OEFS_OK);

        OE_TEST(stat.st_dev == 0);
        OE_TEST(stat.st_ino != 0);
        OE_TEST(stat.st_mode == OEFS_S_DIR_DEFAULT);
        OE_TEST(stat.st_mode & OEFS_S_IFDIR);
        OE_TEST(stat.__st_padding == 0);
        OE_TEST(stat.st_nlink == 1);
        OE_TEST(stat.st_uid == 0);
        OE_TEST(stat.st_gid == 0);
        OE_TEST(stat.st_rdev == 0);
        OE_TEST(stat.st_size > sizeof(oefs_dirent_t) * 2);
        OE_TEST(stat.st_blksize == OEFS_BLOCK_SIZE);
        OE_TEST(stat.st_blocks > 2);
        OE_TEST(stat.st_atime == 0);
        OE_TEST(stat.st_mtime == 0);
        OE_TEST(stat.st_ctime == 0);
    }

    /* Stat the directory and check expectations. */
    {
        oefs_stat_t stat;
        r = oefs_stat(oefs, "/aaa/bbb/ccc", &stat);
        OE_TEST(r == OEFS_OK);

        OE_TEST(stat.st_dev == 0);
        OE_TEST(stat.st_ino != 0);
        OE_TEST(stat.st_mode == OEFS_S_DIR_DEFAULT);
        OE_TEST(stat.st_mode & OEFS_S_IFDIR);
        OE_TEST(stat.__st_padding == 0);
        OE_TEST(stat.st_nlink == 1);
        OE_TEST(stat.st_uid == 0);
        OE_TEST(stat.st_gid == 0);
        OE_TEST(stat.st_rdev == 0);
        OE_TEST(stat.st_size == sizeof(oefs_dirent_t) * 2);
        OE_TEST(stat.st_blksize == OEFS_BLOCK_SIZE);
        OE_TEST(stat.st_blocks == 2);
        OE_TEST(stat.st_atime == 0);
        OE_TEST(stat.st_mtime == 0);
        OE_TEST(stat.st_ctime == 0);
    }
}

static void _remove_dir_nnnn(oefs_t* oefs)
{
    const size_t NUM_DIRS = 10;
    oefs_result_t r;

    for (size_t i = 0; i < NUM_DIRS; i++)
    {
        char name[OEFS_PATH_MAX];
        snprintf(name, sizeof(name), "/dir-%04zu", i);

        r = oefs_rmdir(oefs, name);
        OE_TEST(r == OEFS_OK);
    }
}

static void _dump_file(oefs_t* oefs, const char* path)
{
    void* data;
    size_t size;
    oefs_result_t r;

    r = oefs_load(oefs, path, &data, &size);
    OE_TEST(r == OEFS_OK);

    printf("<<<<<<<< _dump_file(%s): %zu bytes\n", path, size);

    for (size_t i = 0; i < size; i++)
    {
        uint8_t b = *((uint8_t*)data + i);

        if (isprint(b))
            printf("%c", b);
        else
            printf("<%02x>", b);

        if (i + 1 == size)
            printf("\n");
    }

    printf(">>>>>>>>\n\n");

    free(data);
}

static void _update_file(oefs_t* oefs, const char* path)
{
    oefs_result_t r;
    oefs_file_t* file = NULL;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    const size_t FILE_SIZE = 1024;
    buf_t buf = BUF_INITIALIZER;
    int32_t n;

    r = oefs_open(oefs, path, 0, 0, &file);
    OE_TEST(r == OEFS_OK);

    for (size_t i = 0; i < FILE_SIZE; i++)
    {
        char c = alphabet[i % (sizeof(alphabet) - 1)];

        if (buf_append(&buf, &c, 1) != 0)
            OE_TEST(false);
    }

    r = oefs_write(file, buf.data, buf.size, &n);
    OE_TEST(r == OEFS_OK);
    OE_TEST(n == buf.size);
    oefs_close(file);

    /* Load the file to make sure the changes took. */
    {
        void* data;
        size_t size;

        r = oefs_load(oefs, path, &data, &size);
        OE_TEST(r == OEFS_OK);
        OE_TEST(size == buf.size);
        OE_TEST(memcmp(data, buf.data, size) == 0);

        free(data);
    }

    /* Stat the file and check expectations. */
    {
        oefs_stat_t stat;
        r = oefs_stat(oefs, path, &stat);
        OE_TEST(r == OEFS_OK);

        OE_TEST(stat.st_dev == 0);
        OE_TEST(stat.st_ino != 0);
        OE_TEST(stat.st_mode == OEFS_S_REG_DEFAULT);
        OE_TEST(stat.st_mode & OEFS_S_IFREG);
        OE_TEST(stat.__st_padding == 0);
        OE_TEST(stat.st_nlink == 1);
        OE_TEST(stat.st_uid == 0);
        OE_TEST(stat.st_gid == 0);
        OE_TEST(stat.st_rdev == 0);
        OE_TEST(stat.st_size == buf.size);
        OE_TEST(stat.st_blksize == OEFS_BLOCK_SIZE);
        uint32_t n = (buf.size + OEFS_BLOCK_SIZE - 1) / OEFS_BLOCK_SIZE;
        OE_TEST(stat.st_blocks == n);
        OE_TEST(stat.st_atime == 0);
        OE_TEST(stat.st_mtime == 0);
        OE_TEST(stat.st_ctime == 0);
    }

    buf_release(&buf);
}

static void _create_myfile(oefs_t* oefs)
{
    oefs_result_t r;
    oefs_file_t* file;
    const char path[] = "/aaa/bbb/ccc/myfile";

    r = oefs_create(oefs, path, 0, &file);
    OE_TEST(r == OEFS_OK);

    const char message[] = "Hello World!";

    int32_t n;
    r = oefs_write(file, message, sizeof(message), &n);
    OE_TEST(r == OEFS_OK);
    OE_TEST(n == sizeof(message));

    oefs_close(file);
    _dump_file(oefs, path);
}

static void _truncate_file(oefs_t* oefs, const char* path)
{
    oefs_result_t r = oefs_truncate(oefs, path);
    OE_TEST(r == OEFS_OK);
}

static void _remove_file(oefs_t* oefs, const char* path)
{
    oefs_result_t r = oefs_unlink(oefs, path);
    OE_TEST(r == OEFS_OK);
}

static void _remove_dir(oefs_t* oefs, const char* path)
{
    oefs_result_t r = oefs_rmdir(oefs, path);
    OE_TEST(r == OEFS_OK);
}

static void _test_lseek(oefs_t* oefs)
{
    oefs_result_t r;
    oefs_file_t* file;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    const size_t alphabet_length = OE_COUNTOF(alphabet) - 1;
    char buf[1093];
    ssize_t offset;
    int32_t nread;
    int32_t nwritten;

    r = oefs_create(oefs, "/somefile", 0, &file);
    OE_TEST(r == OEFS_OK);

    for (size_t i = 0; i < alphabet_length; i++)
    {
        memset(buf, alphabet[i], sizeof(buf));
        r = oefs_write(file, buf, sizeof(buf), &nwritten);
        OE_TEST(r == OEFS_OK);
        OE_TEST(nwritten == sizeof(buf));
    }

    r = oefs_lseek(file, 0, OEFS_SEEK_CUR, &offset);
    OE_TEST(r == OEFS_OK);
    OE_TEST(offset == sizeof(buf) * 26);

    r = oefs_lseek(file, -2 * sizeof(buf), OEFS_SEEK_CUR, &offset);
    OE_TEST(r == OEFS_OK);

    memset(buf, 0, sizeof(buf));
    r = oefs_read(file, buf, sizeof(buf), &nread);
    OE_TEST(r == OEFS_OK);
    OE_TEST(nread == sizeof(buf));

    {
        char tmp[sizeof(buf)];
        memset(tmp, 'y', sizeof(tmp));
        OE_TEST(memcmp(buf, tmp, sizeof(tmp)) == 0);
    }

    r = oefs_close(file);
    OE_TEST(r == OEFS_OK);

    //_dump_file(oefs, "/somefile");
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
    r = oefs_size(num_blocks, &size);
    OE_TEST(r == OEFS_OK);

    /* Ask the host to open the OEFS file. */
    result = oe_open_host_block_dev(oefs_filename, &dev);
    OE_TEST(result == OE_OK);

    /* Initialize the OEFS file. */
    r = oefs_mkfs(dev, num_blocks);
    OE_TEST(r == OEFS_OK);

    /* Create an new OEFS object. */
    r = oefs_initialize(&oefs, dev);
    OE_TEST(r == OEFS_OK);

    /* Create some files. */
    _create_files(oefs);

    /* Create some directories. */
    _create_dirs(oefs);

    /* Dump some directories. */
    _dump_dir(oefs, "/");
    _dump_dir(oefs, "/dir-0001");
    _dump_dir(oefs, "/aaa/bbb/ccc");

    /* Test updating of a file. */
    {
        const char path[] = "/filename-0001";
        _update_file(oefs, path);
        _dump_file(oefs, path);
        _truncate_file(oefs, path);
        _dump_file(oefs, path);
    }

    /* Create "/aaa/bbb/ccc/myfile" */
    _dump_dir(oefs, "/aaa/bbb/ccc");
    _create_myfile(oefs);
    _dump_dir(oefs, "/aaa/bbb/ccc");
    _remove_file(oefs, "/aaa/bbb/ccc/myfile");
    _dump_dir(oefs, "/aaa/bbb/ccc");

    /* Remove some directories. */
    _dump_dir(oefs, "/aaa/bbb");
    _remove_dir(oefs, "/aaa/bbb/ccc");
    _dump_dir(oefs, "/aaa/bbb");

    /* Remove some directories. */
    _dump_dir(oefs, "/aaa");
    _remove_dir(oefs, "/aaa/bbb");
    _dump_dir(oefs, "/aaa");

    /* Remove some directories. */
    _dump_dir(oefs, "/");
    _remove_dir(oefs, "/aaa");
    _dump_dir(oefs, "/");

    /* Remove directories named like dir-NNNN */
    _remove_dir_nnnn(oefs);
    _dump_dir(oefs, "/");

    /* Remove files named like filename-NNNN */
    _remove_files_nnnn(oefs);
    _dump_dir(oefs, "/");

    /* Test the lseek function. */
    _test_lseek(oefs);

    oefs_release(oefs);
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

