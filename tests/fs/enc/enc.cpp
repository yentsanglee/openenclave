// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "file_system.h"
#include "fs_t.h"

static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
static const oe_mode_t MODE = 0644;

const char* mkpath(char buf[OE_PATH_MAX], const char* target, const char* path)
{
    strlcpy(buf, target, OE_PATH_MAX);
    strlcat(buf, "/", OE_PATH_MAX);
    strlcat(buf, path, OE_PATH_MAX);
    return buf;
}

template <class FILE_SYSTEM>
static void cleanup(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];

    fs.unlink(mkpath(path, tmp_dir, "alphabet"));
    fs.unlink(mkpath(path, tmp_dir, "alphabet.renamed"));

    mkpath(path, tmp_dir, "dir1");
    fs.rmdir(path);

    mkpath(path, tmp_dir, "dir2");
    fs.rmdir(path);
}

template <class FILE_SYSTEM>
static void test_create_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    typename FILE_SYSTEM::file_handle file;

    mkpath(path, tmp_dir, "alphabet");

    /* Open the file for output. */
    {
        const int flags = OE_O_CREAT | OE_O_TRUNC | OE_O_WRONLY;
        OE_TEST(file = fs.open(path, flags, MODE));
    }

    /* Write to the file. */
    {
        ssize_t n = fs.write(file, ALPHABET, sizeof(ALPHABET));
        OE_TEST(n == sizeof(ALPHABET));
    }

    /* Close the file. */
    OE_TEST(fs.close(file) == 0);

    printf("=== Created %s\n", path);
}

template <class FILE_SYSTEM>
static void test_read_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    typename FILE_SYSTEM::file_handle file;
    char buf[OE_PAGE_SIZE];

    mkpath(path, tmp_dir, "alphabet");

    /* Open the file for input. */
    {
        const int flags = OE_O_RDONLY;
        file = fs.open(path, flags, 0);
        OE_TEST(file);
    }

    /* Read the whole file. */
    {
        ssize_t n = fs.read(file, buf, sizeof(buf));
        OE_TEST(n == sizeof(ALPHABET));
        OE_TEST(memcmp(buf, ALPHABET, sizeof(ALPHABET)) == 0);
    }

    /* Read "lmnop" */
    {
        OE_TEST(fs.lseek(file, 11, OE_SEEK_SET) == 11);
        OE_TEST(fs.read(file, buf, 5) == 5);
        OE_TEST(memcmp(buf, "lmnop", 5) == 0);
    }

    /* Read one character at a time. */
    {
        OE_TEST(fs.lseek(file, 0, OE_SEEK_SET) == 0);

        for (size_t i = 0; i < OE_COUNTOF(ALPHABET); i++)
        {
            OE_TEST(fs.read(file, buf, 1) == 1);
            OE_TEST(ALPHABET[i] == buf[0]);
        }
    }

    /* Close the file. */
    OE_TEST(fs.close(file) == 0);
}

template <class FILE_SYSTEM>
static void test_stat_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    mkpath(path, tmp_dir, "alphabet");

    /* Stat the file. */
    OE_TEST(fs.stat(path, &buf) == 0);

    /* Check stats. */
    OE_TEST(buf.st_size == sizeof(ALPHABET));
    OE_TEST(OE_S_ISREG(buf.st_mode));
    OE_TEST(buf.st_mode == (OE_S_IFREG | MODE));
}

template <class FILE_SYSTEM>
static void test_readdir(FILE_SYSTEM& fs, const char* tmp_dir)
{
    typename FILE_SYSTEM::dir_handle dir;
    struct oe_dirent* ent;
    size_t count = 0;
    bool found_alphabet = false;
    bool found_alphabet_renamed = false;
    bool found_dot = false;
    bool found_dot_dot = false;
    bool found_dir1 = false;
    bool found_dir2 = false;
    char path[OE_PATH_MAX];

    dir = fs.opendir(tmp_dir);
    OE_TEST(dir);

    /* Create directories: "dir1" and "dir2". */
    {
        mkpath(path, tmp_dir, "dir1");
        OE_TEST(fs.mkdir(path, 0777) == 0);

        mkpath(path, tmp_dir, "dir2");
        OE_TEST(fs.mkdir(path, 0777) == 0);
    }

    /* Test stat on a directory. */
    {
        struct oe_stat buf;
        OE_TEST(fs.stat(mkpath(path, tmp_dir, "dir1"), &buf) == 0);
        OE_TEST(OE_S_ISDIR(buf.st_mode));
    }

    /* Remove directory "dir2". */
    OE_TEST(fs.rmdir(mkpath(path, tmp_dir, "dir2")) == 0);

    while ((ent = fs.readdir(dir)))
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

    fs.closedir(dir);
}

template <class FILE_SYSTEM>
static void test_link_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    struct oe_stat buf;

    mkpath(oldname, tmp_dir, "alphabet");
    mkpath(newname, tmp_dir, "alphabet.linked");

    OE_TEST(fs.link(oldname, newname) == 0);
    OE_TEST(fs.stat(newname, &buf) == 0);
}

template <class FILE_SYSTEM>
static void test_rename_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    struct oe_stat buf;

    mkpath(oldname, tmp_dir, "alphabet.linked"),
        mkpath(newname, tmp_dir, "alphabet.renamed");

    OE_TEST(fs.rename(oldname, newname) == 0);
    OE_TEST(fs.stat(newname, &buf) == 0);
}

template <class FILE_SYSTEM>
static void test_truncate_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    mkpath(path, tmp_dir, "alphabet");

    /* Remove the file. */
    OE_TEST(fs.truncate(path, 5) == 0);

    /* Stat the file. */
    OE_TEST(fs.stat(path, &buf) == 0);
    OE_TEST(buf.st_size == 5);
}

template <class FILE_SYSTEM>
static void test_unlink_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    struct oe_stat buf;

    mkpath(path, tmp_dir, "alphabet");

    /* Remove the file. */
    OE_TEST(fs.unlink(path) == 0);

    /* Stat the file. */
    OE_TEST(fs.stat(path, &buf) != 0);
}

template <class FILE_SYSTEM>
void test_all(FILE_SYSTEM& fs, const char* tmp_dir)
{
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

void test_fs(const char* src_dir, const char* tmp_dir)
{
    (void)src_dir;

    /* Test the fs_file_system interface. */
    hostfs_file_system fs;
    test_all(fs, tmp_dir);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
