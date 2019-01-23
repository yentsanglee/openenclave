// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/fs_ops.h>
#include <stdio.h>
#include <string.h>
#include "hostfs_t.h"

static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
static const oe_mode_t MODE = 0644;

static const char* _mkpath(
    char buf[OE_PATH_MAX],
    const char* target,
    const char* path)
{
    strlcpy(buf, target, OE_PATH_MAX);
    strlcat(buf, path, OE_PATH_MAX);
    return buf;
}

void cleanup(oe_device_t* hostfs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];

    oe_fs_unlink(hostfs, _mkpath(path, tmp_dir, "/alphabet"));
    oe_fs_unlink(hostfs, _mkpath(path, tmp_dir, "/alphabet.renamed"));
}

void create_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    oe_device_t* file;

    _mkpath(path, tmp_dir, "/alphabet");

    /* Open the file for output. */
    {
        const int flags = OE_O_CREAT | OE_O_TRUNC | OE_O_WRONLY;
        OE_TEST(file = oe_fs_open(hostfs, path, flags, MODE));
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

void read_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    oe_device_t* file;
    char buf[OE_PAGE_SIZE];

    _mkpath(path, tmp_dir, "/alphabet");

    /* Open the file for input. */
    {
        const int flags = OE_O_RDONLY;
        file = oe_fs_open(hostfs, path, flags, 0);
        OE_TEST(file);
    }

    /* Read the file. */
    {
        ssize_t n = oe_fs_read(file, buf, sizeof(buf));
        OE_TEST(n == sizeof(ALPHABET));
        OE_TEST(memcmp(buf, ALPHABET, sizeof(ALPHABET)) == 0);
    }

    /* Close the file. */
    OE_TEST(oe_fs_close(file) == 0);
}

void stat_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(path, tmp_dir, "/alphabet");

    /* Stat the file. */
    OE_TEST(oe_fs_stat(hostfs, path, &buf) == 0);

    /* Check stats. */
    OE_TEST(buf.st_size == sizeof(ALPHABET));
    OE_TEST(OE_S_ISREG(buf.st_mode));
    OE_TEST(buf.st_mode == (OE_S_IFREG | MODE));
}

void find_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    oe_device_t* dir;
    struct oe_dirent* ent;
    size_t count = 0;
    bool found_alphabet = false;
    bool found_alphabet_renamed = false;
    bool found_dot = false;
    bool found_dot_dot = false;

    dir = oe_fs_opendir(hostfs, tmp_dir);
    OE_TEST(dir);

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

        count++;
    }

    OE_TEST(found_alphabet);
    OE_TEST(found_alphabet_renamed);
    OE_TEST(found_dot);
    OE_TEST(found_dot_dot);

    oe_fs_closedir(dir);
}

void link_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(oldname, tmp_dir, "/alphabet");
    _mkpath(newname, tmp_dir, "/alphabet.linked");

    OE_TEST(oe_fs_link(hostfs, oldname, newname) == 0);
    OE_TEST(oe_fs_stat(hostfs, newname, &buf) == 0);
}

void rename_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(oldname, tmp_dir, "/alphabet.linked"),
    _mkpath(newname, tmp_dir, "/alphabet.renamed");

    OE_TEST(oe_fs_rename(hostfs, oldname, newname) == 0);
    OE_TEST(oe_fs_stat(hostfs, newname, &buf) == 0);
}

void unlink_alphabet_file(oe_device_t* hostfs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    _mkpath(path, tmp_dir, "/alphabet");

    /* Remove the file. */
    OE_TEST(oe_fs_unlink(hostfs, path) == 0);

    /* Stat the file. */
    OE_TEST(oe_fs_stat(hostfs, path, &buf) != 0);
}

void test_hostfs(const char* src_dir, const char* tmp_dir)
{
    (void)src_dir;

    oe_device_t* hostfs = oe_fs_get_hostfs();
    OE_TEST(hostfs);

    cleanup(hostfs, tmp_dir);
    create_alphabet_file(hostfs, tmp_dir);
    read_alphabet_file(hostfs, tmp_dir);
    stat_alphabet_file(hostfs, tmp_dir);
    link_alphabet_file(hostfs, tmp_dir);
    rename_alphabet_file(hostfs, tmp_dir);
    find_alphabet_file(hostfs, tmp_dir);
    unlink_alphabet_file(hostfs, tmp_dir);
    cleanup(hostfs, tmp_dir);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
