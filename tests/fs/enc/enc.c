// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "fs_t.h"

static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
static const oe_mode_t MODE = 0644;

static const char* _mkpath(
    char buf[OE_PATH_MAX],
    const char* target,
    const char* path)
{
    strlcpy(buf, target, OE_PATH_MAX);
    strlcat(buf, "/", OE_PATH_MAX);
    strlcat(buf, path, OE_PATH_MAX);
    return buf;
}

void cleanup(oe_device_t* fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];

    oe_fs_unlink(fs, _mkpath(path, tmp_dir, "alphabet"));
    oe_fs_unlink(fs, _mkpath(path, tmp_dir, "alphabet.renamed"));

    _mkpath(path, tmp_dir, "dir1");
    oe_fs_rmdir(fs, path);

    _mkpath(path, tmp_dir, "dir2");
    oe_fs_rmdir(fs, path);
}

void test_create_file(oe_device_t* fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    oe_device_t* file;

    _mkpath(path, tmp_dir, "alphabet");

    /* Open the file for output. */
    {
        const int flags = OE_O_CREAT | OE_O_TRUNC | OE_O_WRONLY;
        OE_TEST(file = oe_fs_open(fs, path, flags, MODE));
    }

    /* Write to the file. */
    {
        ssize_t n = oe_fs_write(file, ALPHABET, sizeof(ALPHABET));
        OE_TEST(n == sizeof(ALPHABET));
    }

    /* Close the file. */
    OE_TEST(oe_fs_close(file) == 0);

    printf("=== Created %s\n", path);
}

void test_read_file(oe_device_t* fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    oe_device_t* file;
    char buf[OE_PAGE_SIZE];

    _mkpath(path, tmp_dir, "alphabet");

    /* Open the file for input. */
    {
        const int flags = OE_O_RDONLY;
        file = oe_fs_open(fs, path, flags, 0);
        OE_TEST(file);
    }

    /* Read the whole file. */
    {
        ssize_t n = oe_fs_read(file, buf, sizeof(buf));
        OE_TEST(n == sizeof(ALPHABET));
        OE_TEST(memcmp(buf, ALPHABET, sizeof(ALPHABET)) == 0);
    }

    /* Read "lmnop" */
    {
        OE_TEST(oe_fs_lseek(file, 11, OE_SEEK_SET) == 11);
        OE_TEST(oe_fs_read(file, buf, 5) == 5);
        OE_TEST(memcmp(buf, "lmnop", 5) == 0);
    }

    /* Read one character at a time. */
    {
        OE_TEST(oe_fs_lseek(file, 0, OE_SEEK_SET) == 0);

        for (size_t i = 0; i < OE_COUNTOF(ALPHABET); i++)
        {
            OE_TEST(oe_fs_read(file, buf, 1) == 1);
            OE_TEST(ALPHABET[i] == buf[0]);
        }
    }

    /* Close the file. */
    OE_TEST(oe_fs_close(file) == 0);
}

void test_stat_file(oe_device_t* fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(path, tmp_dir, "alphabet");

    /* Stat the file. */
    OE_TEST(oe_fs_stat(fs, path, &buf) == 0);

    /* Check stats. */
    OE_TEST(buf.st_size == sizeof(ALPHABET));
    OE_TEST(OE_S_ISREG(buf.st_mode));
    OE_TEST(buf.st_mode == (OE_S_IFREG | MODE));
}

void test_readdir(oe_device_t* fs, const char* tmp_dir)
{
    oe_device_t* dir;
    struct oe_dirent* ent;
    size_t count = 0;
    bool found_alphabet = false;
    bool found_alphabet_renamed = false;
    bool found_dot = false;
    bool found_dot_dot = false;
    bool found_dir1 = false;
    bool found_dir2 = false;
    char path[OE_PATH_MAX];

    dir = oe_fs_opendir(fs, tmp_dir);
    OE_TEST(dir);

    /* Create directories: "dir1" and "dir2". */
    {
        _mkpath(path, tmp_dir, "dir1");
        OE_TEST(oe_fs_mkdir(fs, path, 0777) == 0);

        _mkpath(path, tmp_dir, "dir2");
        OE_TEST(oe_fs_mkdir(fs, path, 0777) == 0);
    }

    /* Test stat on a directory. */
    {
        struct oe_stat buf;
        OE_TEST(oe_fs_stat(fs, _mkpath(path, tmp_dir, "dir1"), &buf) == 0);
        OE_TEST(OE_S_ISDIR(buf.st_mode));
    }

    /* Remove directory "dir2". */
    OE_TEST(oe_fs_rmdir(fs, _mkpath(path, tmp_dir, "dir2")) == 0);

    while ((ent = oe_fs_readdir(dir)))
    {
        if (strcmp(ent->d_name, "alphabet") == 0)
            found_alphabet = true;

        if (strcmp(ent->d_name, "alphabet.renamed") == 0)
            found_alphabet_renamed = true;

        if (strcmp(ent->d_name, ".") == 0)
            found_dot = true;

        if (strcmp(ent->d_name, "..") == 0)
            found_dot_dot = true;

        if (strcmp(ent->d_name, "dir1") == 0)
            found_dir1 = true;

        if (strcmp(ent->d_name, "dir2") == 0)
            found_dir2 = true;

        count++;
    }

    OE_TEST(found_alphabet);
    OE_TEST(found_alphabet_renamed);
    OE_TEST(found_dot);
    OE_TEST(found_dot_dot);
    OE_TEST(found_dir1);
    OE_TEST(!found_dir2);

    oe_fs_closedir(dir);
}

void test_link_file(oe_device_t* fs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(oldname, tmp_dir, "alphabet");
    _mkpath(newname, tmp_dir, "alphabet.linked");

    OE_TEST(oe_fs_link(fs, oldname, newname) == 0);
    OE_TEST(oe_fs_stat(fs, newname, &buf) == 0);
}

void test_rename_file(oe_device_t* fs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(oldname, tmp_dir, "alphabet.linked"),
    _mkpath(newname, tmp_dir, "alphabet.renamed");

    OE_TEST(oe_fs_rename(fs, oldname, newname) == 0);
    OE_TEST(oe_fs_stat(fs, newname, &buf) == 0);
}

void test_truncate_file(oe_device_t* fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(path, tmp_dir, "alphabet");

    /* Remove the file. */
    OE_TEST(oe_fs_truncate(fs, path, 5) == 0);

    /* Stat the file. */
    OE_TEST(oe_fs_stat(fs, path, &buf) == 0);
    OE_TEST(buf.st_size == 5);
}

void test_unlink_file(oe_device_t* fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(path, tmp_dir, "alphabet");

    /* Remove the file. */
    OE_TEST(oe_fs_unlink(fs, path) == 0);

    /* Stat the file. */
    OE_TEST(oe_fs_stat(fs, path, &buf) != 0);
}

void test_fs(const char* src_dir, const char* tmp_dir)
{
    (void)src_dir;
    oe_device_t* fs;

    OE_TEST(oe_fs_clone(oe_fs_get_hostfs(), &fs) == 0);

    cleanup(fs, tmp_dir);
    test_create_file(fs, tmp_dir);
    test_read_file(fs, tmp_dir);
    test_stat_file(fs, tmp_dir);
    test_link_file(fs, tmp_dir);
    test_rename_file(fs, tmp_dir);
    test_readdir(fs, tmp_dir);
    test_truncate_file(fs, tmp_dir);
    test_unlink_file(fs, tmp_dir);
    cleanup(fs, tmp_dir);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
