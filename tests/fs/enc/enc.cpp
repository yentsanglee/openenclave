// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/device.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/fs_ops.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "file_system.h"
#include "fs_t.h"

static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";
static const mode_t MODE = 0644;

const char* mkpath(char buf[OE_PATH_MAX], const char* target, const char* path)
{
    strlcpy(buf, target, OE_PATH_MAX);
    strlcat(buf, "/", OE_PATH_MAX);
    strlcat(buf, path, OE_PATH_MAX);
    return buf;
}

class device_registrant
{
  public:
    device_registrant(oe_devid_t devid)
    {
        OE_TEST(oe_set_thread_default_device(devid) == 0);
    }

    ~device_registrant()
    {
        OE_TEST(oe_clear_thread_default_device() == 0);
    }
};

template <class FILE_SYSTEM>
static void cleanup(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];

    fs.unlink(mkpath(path, tmp_dir, "alphabet"));
    fs.unlink(mkpath(path, tmp_dir, "alphabet.renamed"));
    fs.unlink(mkpath(path, tmp_dir, "alphabet.linked"));

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

    printf("--- %s()\n", __FUNCTION__);

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
}

template <class FILE_SYSTEM>
static void test_read_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    typename FILE_SYSTEM::file_handle file;
    char buf[OE_PAGE_SIZE];

    printf("--- %s()\n", __FUNCTION__);

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
    typename FILE_SYSTEM::stat_type buf;

    printf("--- %s()\n", __FUNCTION__);

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
    typename FILE_SYSTEM::dirent_type* ent;
    size_t count = 0;
    bool found_alphabet = false;
    bool found_alphabet_renamed = false;
    bool found_dot = false;
    bool found_dot_dot = false;
    bool found_dir1 = false;
    bool found_dir2 = false;
    char path[OE_PATH_MAX];

    printf("--- %s()\n", __FUNCTION__);

    /* Create directories: "dir1" and "dir2". */
    {
        mkpath(path, tmp_dir, "dir1");
        OE_TEST(fs.mkdir(path, 0777) == 0);

        mkpath(path, tmp_dir, "dir2");
        OE_TEST(fs.mkdir(path, 0777) == 0);
    }

    /* Test stat on a directory. */
    {
        typename FILE_SYSTEM::stat_type buf;
        OE_TEST(fs.stat(mkpath(path, tmp_dir, "dir1"), &buf) == 0);
        OE_TEST(OE_S_ISDIR(buf.st_mode));
    }

    /* Remove directory "dir2". */
    OE_TEST(fs.rmdir(mkpath(path, tmp_dir, "dir2")) == 0);

    dir = fs.opendir(tmp_dir);
    OE_TEST(dir);

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
    typename FILE_SYSTEM::stat_type buf;

    printf("--- %s()\n", __FUNCTION__);

    mkpath(oldname, tmp_dir, "alphabet");
    mkpath(newname, tmp_dir, "alphabet.linked");

    OE_TEST(fs.link(oldname, newname) == 0);
    OE_TEST(fs.stat(oldname, &buf) == 0);
    OE_TEST(fs.stat(newname, &buf) == 0);
}

template <class FILE_SYSTEM>
static void test_rename_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char oldname[OE_PAGE_SIZE];
    char newname[OE_PAGE_SIZE];
    typename FILE_SYSTEM::stat_type buf;

    printf("--- %s()\n", __FUNCTION__);

    mkpath(oldname, tmp_dir, "alphabet.linked"),
        mkpath(newname, tmp_dir, "alphabet.renamed");

    OE_TEST(fs.rename(oldname, newname) == 0);
    OE_TEST(fs.stat(oldname, &buf) != 0);
    OE_TEST(fs.stat(newname, &buf) == 0);
}

template <class FILE_SYSTEM>
static void test_truncate_file(FILE_SYSTEM& fs, const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    typename FILE_SYSTEM::stat_type buf;

    printf("--- %s()\n", __FUNCTION__);

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
    typename FILE_SYSTEM::stat_type buf;

    printf("--- %s()\n", __FUNCTION__);

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

void test_fprintf_fscanf(const char* tmp_dir)
{
    char path[OE_PAGE_SIZE];
    device_registrant registrant(OE_DEVID_HOSTFS);

    printf("--- %s()\n", __FUNCTION__);

    mkpath(path, tmp_dir, "fprintf");

    /* Write "5 five" to the file. */
    {
        FILE* stream;
        OE_TEST((stream = fopen(path, "w")));
        int n = fprintf(stream, "%d %s", 5, "five");
        OE_TEST(n == 6);
        OE_TEST(fclose(stream) == 0);
    }

    /* Read back the file. */
    {
        FILE* stream;
        int num;
        char str[16];

        OE_TEST((stream = fopen(path, "r")));
        int n = fscanf(stream, "%d %s", &num, str);
        OE_TEST(n == 2);
        OE_TEST(num = 5);
        OE_TEST(strcmp(str, "five") == 0);
        OE_TEST(fclose(stream) == 0);
    }
}

void test_fs(const char* src_dir, const char* tmp_dir)
{
    (void)src_dir;

    printf("=== running all tests\n");
    printf("--- src_dir=%s\n", src_dir);
    printf("--- tmp_dir=%s\n", tmp_dir);

    /* Test the hostfs file system. */
    {
        printf("=== testing hostfs:\n");
        hostfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test the sgxfs file system. */
    {
        printf("=== testing sgxfs:\n");
        sgxfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test the HOSTFS oe file descriptor interfaces. */
    {
        printf("=== testing oe-fd-hostfs:\n");

        oe_fd_hostfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test the SGXFS oe file descriptor interfaces. */
    {
        printf("=== testing oe-fd-sgxfs:\n");

        oe_fd_sgxfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test the HOSTFS standard C descriptor interfaces. */
    {
        printf("=== testing fd-hostfs:\n");

        fd_hostfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test the SGXFS standard C descriptor interfaces. */
    {
        printf("=== testing fd-sgxfs:\n");

        fd_sgxfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test stream I/O hostfs functions. */
    {
        printf("=== testing stream I/O hostfs functions:\n");

        stream_hostfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test stream I/O sgxfs functions. */
    {
        printf("=== testing stream I/O sgxfs functions:\n");

        stream_sgxfs_file_system fs;
        test_all(fs, tmp_dir);
    }

    /* Test oe_set_thread_default_device() */
    {
        printf("=== testing oe_set_thread_default_device:\n");

        oe_register_hostfs_device();
        fd_file_system fs;
        OE_TEST(oe_set_thread_default_device(OE_DEVID_HOSTFS) == 0);
        OE_TEST(oe_get_thread_default_device() == OE_DEVID_HOSTFS);
        test_all(fs, tmp_dir);
        OE_TEST(oe_clear_thread_default_device() == 0);
    }

    /* Test writing to a read-only mounted file system. */
    {
        char path[OE_PATH_MAX];
        mkpath(path, tmp_dir, "somefile");
        const int flags = OE_O_CREAT | OE_O_TRUNC | OE_O_WRONLY;

        OE_TEST(oe_mount(OE_DEVID_HOSTFS, NULL, "/", OE_MOUNT_RDONLY) == 0);
        OE_TEST(oe_open(OE_DEVID_NULL, path, flags, MODE) == -1);
        OE_TEST(oe_errno == OE_EPERM);
        OE_TEST(oe_unmount(OE_DEVID_HOSTFS, "/") == 0);
    }

    /* Test iot I/O. */
    {
        printf("=== testing iot:\n");
        extern void test_iot(const char* tmp_dir);
        test_iot(tmp_dir);
    }

    /* Write the standard output and standard error. */
    {
        static const char DATA[] = "abcdefghijklmnopqrstuvwxyz\n";
        static const size_t n = sizeof(DATA) - 1;
        OE_TEST(oe_write(OE_STDOUT_FILENO, DATA, n) == n);
        OE_TEST(oe_write(OE_STDERR_FILENO, DATA, n) == n);
    }

    /* Test fprintf and fscanf. */
    test_fprintf_fscanf(tmp_dir);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
