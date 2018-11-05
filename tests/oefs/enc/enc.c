// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/file.h>
#include <openenclave/internal/tests.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../../fs/buf.h"
#include "../../../fs/fs.h"
#include "../../../fs/mount.h"

#define INIT

#define BLOCK_SIZE 512

static int _load(fs_t* fs, const char* path, void** data_out, size_t* size_out)
{
    int ret = -1;
    fs_file_t* file = NULL;
    fs_buf_t buf = BUF_INITIALIZER;

    if (data_out)
        *data_out = NULL;

    if (size_out)
        *size_out = 0;

    if (!fs || !path || !data_out || !size_out)
        goto done;

    if (fs->fs_open(fs, path, 0, 0, &file))
        goto done;

    for (;;)
    {
        char data[4096];
        ssize_t n;

        if (fs->fs_read(file, data, sizeof(data), &n) != FS_EOK)
            goto done;

        if (n == 0)
            break;

        if (fs_buf_append(&buf, data, n) != 0)
            goto done;
    }

    *data_out = buf.data;
    *size_out = buf.size;
    memset(&buf, 0, sizeof(buf));

    ret = 0;

done:

    if (file)
        fs->fs_close(file);

    fs_buf_release(&buf);

    return ret;
}

static void _dump_dir(fs_t* fs, const char* dirname)
{
    fs_errno_t r;
    fs_dir_t* dir;
    fs_dirent_t* ent;

    r = fs->fs_opendir(fs, dirname, &dir);
    OE_TEST(r == FS_EOK);
    OE_TEST(dir != NULL);

    printf("<<<<<<<< _dump_dir(%s)\n", dirname);

    while ((r = fs->fs_readdir(dir, &ent)) == FS_EOK && ent)
    {
        printf("name=%s\n", ent->d_name);
    }

    OE_TEST(r == FS_EOK);

    printf(">>>>>>>>\n\n");

    r = fs->fs_closedir(dir);
    OE_TEST(r == FS_EOK);
}

static void _create_files(fs_t* fs)
{
    const size_t NUM_FILES = 100;
    fs_errno_t r;
    fs_file_t* file;

    for (size_t i = 0; i < NUM_FILES; i++)
    {
        char path[FS_PATH_MAX];
        snprintf(path, sizeof(path), "/filename-%04zu", i);
        r = fs->fs_creat(fs, path, 0, &file);
        OE_TEST(r == FS_EOK);
        OE_TEST(file != NULL);
        fs->fs_close(file);
    }
}

static void _remove_files_nnnn(fs_t* fs)
{
    const size_t NUM_FILES = 100;
    fs_errno_t r;

    for (size_t i = 0; i < NUM_FILES; i++)
    {
        char path[FS_PATH_MAX];
        snprintf(path, sizeof(path), "/filename-%04zu", i);
        r = fs->fs_unlink(fs, path);
        OE_TEST(r == FS_EOK);
    }
}

static void _create_dirs(fs_t* fs)
{
    const size_t NUM_DIRS = 10;
    fs_errno_t r;

    for (size_t i = 0; i < NUM_DIRS; i++)
    {
        char name[FS_PATH_MAX];
        snprintf(name, sizeof(name), "/dir-%04zu", i);

        r = fs->fs_mkdir(fs, name, 0);
        OE_TEST(r == FS_EOK);
    }

    r = fs->fs_mkdir(fs, "/aaa", 0);
    OE_TEST(r == FS_EOK);

    r = fs->fs_mkdir(fs, "/aaa/bbb", 0);
    OE_TEST(r == FS_EOK);

    r = fs->fs_mkdir(fs, "/aaa/bbb/ccc", 0);
    OE_TEST(r == FS_EOK);

    /* Stat root directory and check expectations. */
    {
        fs_stat_t stat;
        r = fs->fs_stat(fs, "/", &stat);
        OE_TEST(r == FS_EOK);

        OE_TEST(stat.st_dev == 0);
        OE_TEST(stat.st_ino != 0);
        OE_TEST(stat.st_mode == FS_S_DIR_DEFAULT);
        OE_TEST(stat.st_mode & FS_S_IFDIR);
        OE_TEST(stat.__st_padding1 == 0);
        OE_TEST(stat.st_nlink == 1);
        OE_TEST(stat.st_uid == 0);
        OE_TEST(stat.st_gid == 0);
        OE_TEST(stat.st_rdev == 0);
        OE_TEST(stat.st_size > sizeof(fs_dirent_t) * 2);
        OE_TEST(stat.st_blksize == FS_BLOCK_SIZE);
        OE_TEST(stat.st_blocks > 2);
        OE_TEST(stat.__st_padding2 == 0);
        OE_TEST(stat.st_atim.tv_sec == 0);
        OE_TEST(stat.st_mtim.tv_sec == 0);
        OE_TEST(stat.st_ctim.tv_sec == 0);
    }

    /* Stat the directory and check expectations. */
    {
        fs_stat_t stat;
        r = fs->fs_stat(fs, "/aaa/bbb/ccc", &stat);
        OE_TEST(r == FS_EOK);

        OE_TEST(stat.st_dev == 0);
        OE_TEST(stat.st_ino != 0);
        OE_TEST(stat.st_mode == FS_S_DIR_DEFAULT);
        OE_TEST(stat.st_mode & FS_S_IFDIR);
        OE_TEST(stat.__st_padding1 == 0);
        OE_TEST(stat.st_nlink == 1);
        OE_TEST(stat.st_uid == 0);
        OE_TEST(stat.st_gid == 0);
        OE_TEST(stat.st_rdev == 0);
        OE_TEST(stat.st_size == sizeof(fs_dirent_t) * 2);
        OE_TEST(stat.st_blksize == FS_BLOCK_SIZE);
        OE_TEST(stat.st_blocks == 2);
        OE_TEST(stat.__st_padding1 == 0);
        OE_TEST(stat.st_atim.tv_sec == 0);
        OE_TEST(stat.st_mtim.tv_sec == 0);
        OE_TEST(stat.st_ctim.tv_sec == 0);
    }
}

static void _remove_dir_nnnn(fs_t* fs)
{
    const size_t NUM_DIRS = 10;
    fs_errno_t r;

    for (size_t i = 0; i < NUM_DIRS; i++)
    {
        char name[FS_PATH_MAX];
        snprintf(name, sizeof(name), "/dir-%04zu", i);

        r = fs->fs_rmdir(fs, name);
        OE_TEST(r == FS_EOK);
    }
}

static void _dump_file(fs_t* fs, const char* path)
{
    void* data;
    size_t size;

    OE_TEST(_load(fs, path, &data, &size) == 0);

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

static void _update_file(fs_t* fs, const char* path)
{
    fs_errno_t r;
    fs_file_t* file = NULL;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    const size_t FILE_SIZE = 1024;
    fs_buf_t buf = BUF_INITIALIZER;
    ssize_t n;

    r = fs->fs_open(fs, path, 0, 0, &file);
    OE_TEST(r == FS_EOK);

    for (size_t i = 0; i < FILE_SIZE; i++)
    {
        char c = alphabet[i % (sizeof(alphabet) - 1)];

        if (fs_buf_append(&buf, &c, 1) != 0)
            OE_TEST(false);
    }

    r = fs->fs_write(file, buf.data, buf.size, &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == buf.size);
    fs->fs_close(file);

    /* Load the file to make sure the changes took. */
    {
        void* data;
        size_t size;

        OE_TEST(_load(fs, path, &data, &size) == 0);
        OE_TEST(size == buf.size);
        OE_TEST(memcmp(data, buf.data, size) == 0);

        free(data);
    }

    /* Stat the file and check expectations. */
    {
        fs_stat_t stat;
        r = fs->fs_stat(fs, path, &stat);
        OE_TEST(r == FS_EOK);

        OE_TEST(stat.st_dev == 0);
        OE_TEST(stat.st_ino != 0);
        OE_TEST(stat.st_mode == FS_S_REG_DEFAULT);
        OE_TEST(stat.st_mode & FS_S_IFREG);
        OE_TEST(stat.__st_padding1 == 0);
        OE_TEST(stat.st_nlink == 1);
        OE_TEST(stat.st_uid == 0);
        OE_TEST(stat.st_gid == 0);
        OE_TEST(stat.st_rdev == 0);
        OE_TEST(stat.st_size == buf.size);
        OE_TEST(stat.st_blksize == FS_BLOCK_SIZE);
        ssize_t n = (buf.size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        OE_TEST(stat.st_blocks == n);
        OE_TEST(stat.__st_padding2 == 0);
        OE_TEST(stat.st_atim.tv_sec == 0);
        OE_TEST(stat.st_mtim.tv_sec == 0);
        OE_TEST(stat.st_ctim.tv_sec == 0);
    }

    fs_buf_release(&buf);
}

static void _create_myfile(fs_t* fs)
{
    fs_errno_t r;
    fs_file_t* file;
    const char path[] = "/aaa/bbb/ccc/myfile";

    r = fs->fs_creat(fs, path, 0, &file);
    OE_TEST(r == FS_EOK);

    const char message[] = "Hello World!";

    ssize_t n;
    r = fs->fs_write(file, message, sizeof(message), &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == sizeof(message));

    fs->fs_close(file);
    _dump_file(fs, path);
}

static void _truncate_file(fs_t* fs, const char* path)
{
    fs_errno_t r = fs->fs_truncate(fs, path, 0);
    OE_TEST(r == FS_EOK);
}

static void _remove_file(fs_t* fs, const char* path)
{
    fs_errno_t r = fs->fs_unlink(fs, path);
    OE_TEST(r == FS_EOK);
}

static void _remove_dir(fs_t* fs, const char* path)
{
    fs_errno_t r = fs->fs_rmdir(fs, path);
    OE_TEST(r == FS_EOK);
}

static void _test_lseek(fs_t* fs)
{
    fs_errno_t r;
    fs_file_t* file;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    const size_t alphabet_length = OE_COUNTOF(alphabet) - 1;
    char buf[1093];
    ssize_t offset;
    ssize_t nread;
    ssize_t nwritten;

    r = fs->fs_creat(fs, "/somefile", 0, &file);
    OE_TEST(r == FS_EOK);

    for (size_t i = 0; i < alphabet_length; i++)
    {
        memset(buf, alphabet[i], sizeof(buf));
        r = fs->fs_write(file, buf, sizeof(buf), &nwritten);
        OE_TEST(r == FS_EOK);
        OE_TEST(nwritten == sizeof(buf));
    }

    r = fs->fs_lseek(file, 0, FS_SEEK_CUR, &offset);
    OE_TEST(r == FS_EOK);
    OE_TEST(offset == sizeof(buf) * 26);

    r = fs->fs_lseek(file, -2 * sizeof(buf), FS_SEEK_CUR, &offset);
    OE_TEST(r == FS_EOK);

    memset(buf, 0, sizeof(buf));
    r = fs->fs_read(file, buf, sizeof(buf), &nread);
    OE_TEST(r == FS_EOK);
    OE_TEST(nread == sizeof(buf));

    {
        char tmp[sizeof(buf)];
        memset(tmp, 'y', sizeof(tmp));
        OE_TEST(memcmp(buf, tmp, sizeof(tmp)) == 0);
    }

    r = fs->fs_close(file);
    OE_TEST(r == FS_EOK);

    //_dump_file(fs, "/somefile");
}

static void _test_links(fs_t* fs)
{
    fs_errno_t r;
    fs_file_t* file;
    ssize_t n;
    char buf[1024];
    fs_stat_t stat;

    r = fs->fs_mkdir(fs, "/dir1", 0);
    OE_TEST(r == FS_EOK);

    r = fs->fs_mkdir(fs, "/dir2", 0);
    OE_TEST(r == FS_EOK);

    r = fs->fs_creat(fs, "/dir1/file1", 0, &file);
    OE_TEST(r == FS_EOK);
    r = fs->fs_write(file, "abcdefghijklmnopqrstuvwxyz", 26, &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == 26);
    fs->fs_close(file);

    r = fs->fs_creat(fs, "/dir2/exists", 0, &file);
    OE_TEST(r == FS_EOK);
    fs->fs_close(file);

    r = fs->fs_link(fs, "/dir1/file1", "/dir2/file2");
    OE_TEST(r == FS_EOK);

    r = fs->fs_link(fs, "/dir1/file1", "/dir2/exists");
    OE_TEST(r == FS_EOK);

    r = fs->fs_open(fs, "/dir2/file2", 0, 0, &file);
    OE_TEST(r == FS_EOK);
    r = fs->fs_read(file, buf, sizeof(buf), &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == 26);
    OE_TEST(strncmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);
    fs->fs_close(file);

    r = fs->fs_open(fs, "/dir2/exists", 0, 0, &file);
    OE_TEST(r == FS_EOK);
    r = fs->fs_read(file, buf, sizeof(buf), &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == 26);
    OE_TEST(strncmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);
    fs->fs_close(file);

    r = fs->fs_stat(fs, "/dir1/file1", &stat);
    OE_TEST(r == FS_EOK);
    OE_TEST(stat.st_nlink == 3);

    r = fs->fs_unlink(fs, "/dir2/file2");
    OE_TEST(r == FS_EOK);
    r = fs->fs_stat(fs, "/dir1/file1", &stat);
    OE_TEST(r == FS_EOK);
    OE_TEST(stat.st_nlink == 2);

    r = fs->fs_unlink(fs, "/dir2/exists");
    OE_TEST(r == FS_EOK);
    r = fs->fs_stat(fs, "/dir1/file1", &stat);
    OE_TEST(r == FS_EOK);
    OE_TEST(stat.st_nlink == 1);

    r = fs->fs_unlink(fs, "/dir1/file1");
    OE_TEST(r == FS_EOK);

    fs->fs_rmdir(fs, "/dir1");
    OE_TEST(r == FS_EOK);

    fs->fs_rmdir(fs, "/dir2");
    OE_TEST(r == FS_EOK);
}

static void _test_rename(fs_t* fs, const char* target)
{
    fs_errno_t r;
    fs_file_t* file;
    ssize_t n;
    char buf[1024];
    fs_stat_t stat;

    r = fs->fs_mkdir(fs, "/dir1", 0);
    OE_TEST(r == FS_EOK);

    r = fs->fs_mkdir(fs, "/dir2", 0);
    OE_TEST(r == FS_EOK);

    r = fs->fs_creat(fs, "/dir1/file1", 0, &file);
    OE_TEST(r == FS_EOK);
    r = fs->fs_write(file, "abcdefghijklmnopqrstuvwxyz", 26, &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == 26);
    fs->fs_close(file);

    r = fs->fs_stat(fs, "/dir1/file1", &stat);
    OE_TEST(r == FS_EOK);

    r = fs->fs_rename(fs, "/dir1/file1", "/dir2/file2");
    OE_TEST(r == FS_EOK);

    r = fs->fs_stat(fs, "/dir2/file2", &stat);
    OE_TEST(r == FS_EOK);

    r = fs->fs_open(fs, "/dir2/file2", 0, 0, &file);
    OE_TEST(r == FS_EOK);
    r = fs->fs_read(file, buf, sizeof(buf), &n);
    OE_TEST(r == FS_EOK);
    OE_TEST(n == 26);
    OE_TEST(strncmp(buf, "abcdefghijklmnopqrstuvwxyz", 26) == 0);
    fs->fs_close(file);

    r = fs->fs_stat(fs, "/dir1/file1", &stat);
    OE_TEST(r != FS_EOK);

    /* Test renaming back to original name. */
    {
        char oldpath[FS_PATH_MAX];
        char newpath[FS_PATH_MAX];

        strlcpy(oldpath, target, sizeof(oldpath));
        strlcat(oldpath, "/dir2/file2", sizeof(oldpath));

        strlcpy(newpath, target, sizeof(newpath));
        strlcat(newpath, "/dir1/file1", sizeof(newpath));

        OE_TEST(link(oldpath, newpath) == 0);

        r = fs->fs_stat(fs, "/dir1/file1", &stat);
        OE_TEST(r == FS_EOK);
    }
}

static void _read_alphabet_file(const char* target, const char* path)
{
    char filename[FS_PATH_MAX];
    const size_t FILE_SIZE = 1024;

    strlcpy(filename, target, sizeof(filename));
    strlcat(filename, path, sizeof(filename));

    FILE* is = fopen(filename, "rb");
    OE_TEST(is != NULL);
    ssize_t m = 0;

    for (;;)
    {
        char buf[26];
        ssize_t n = fread(buf, 1, sizeof(buf), is);
        const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";

        OE_TEST(n >= 0);

        if (n == 0)
            break;

        OE_TEST(memcmp(buf, alphabet, n) == 0);

        m += n;
    }

    OE_TEST(m == FILE_SIZE);

    fclose(is);

    /* Stat the file. */
    {
        struct stat st;
        OE_TEST(stat(filename, &st) == 0);
        OE_TEST(st.st_size == FILE_SIZE);
    }
}

static void _write_alphabet_file(const char* target, const char* path)
{
    char filename[FS_PATH_MAX];
    const size_t FILE_SIZE = 1024;
    fs_buf_t buf = BUF_INITIALIZER;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    FILE* is;

    strlcpy(filename, target, sizeof(filename));
    strlcat(filename, path, sizeof(filename));

    is = fopen(filename, "w");
    OE_TEST(is != NULL);

    /* Create buffer to write to file. */
    for (size_t i = 0; i < FILE_SIZE; i++)
    {
        char c = alphabet[i % (sizeof(alphabet) - 1)];

        if (fs_buf_append(&buf, &c, 1) != 0)
            OE_TEST(false);
    }

    ssize_t n = fwrite(buf.data, 1, buf.size, is);
    OE_TEST(n == buf.size);

    fclose(is);
    fs_buf_release(&buf);
}

static void _test_truncate(const char* target)
{
    const size_t file_size = 188416;
    FILE* os;
    char path[FS_PATH_MAX];
    struct stat st;

    strlcpy(path, target, sizeof(path));
    strlcat(path, "/bigfile", sizeof(path));

    /* Write the file */
    {
        OE_TEST((os = fopen(path, "w")) != NULL);

        for (size_t i = 0; i < file_size; i++)
        {
            uint8_t byte = i % 256;

            if (fwrite(&byte, 1, 1, os) != 1)
                OE_TEST(false);
        }

        fclose(os);
    }

    /* Check the file size. */
    OE_TEST(stat(path, &st) == 0);
    OE_TEST(st.st_size = file_size);

    /* Read the file */
    {
        size_t n = 0;

        OE_TEST((os = fopen(path, "r")) != NULL);

        for (size_t i = 0; i < file_size; i++)
        {
            const uint8_t expect = i % 256;
            uint8_t byte;

            if (fread(&byte, 1, 1, os) != 1)
                OE_TEST(false);

            OE_TEST(byte == expect);
            n++;
        }

        fclose(os);

        OE_TEST(n == file_size);
    }

    /* Truncate the file. */
    {
        OE_TEST(truncate(path, file_size / 2) == 0);
        OE_TEST(stat(path, &st) == 0);
        OE_TEST(st.st_size == file_size / 2);

        OE_TEST(truncate(path, file_size / 4) == 0);
        OE_TEST(stat(path, &st) == 0);
        OE_TEST(st.st_size == file_size / 4);

        OE_TEST(truncate(path, file_size / 8) == 0);
        OE_TEST(stat(path, &st) == 0);
        OE_TEST(st.st_size == file_size / 8);
    }

    /* Read the file */
    {
        size_t n = 0;
        uint8_t byte;

        OE_TEST((os = fopen(path, "r")) != NULL);

        for (size_t i = 0; i < file_size / 8; i++)
        {
            const uint8_t expect = i % 256;

            if (fread(&byte, 1, 1, os) != 1)
                OE_TEST(false);

            OE_TEST(byte == expect);
            n++;
        }

        OE_TEST(fread(&byte, 1, 1, os) == 0);
        OE_TEST(feof(os));
        fclose(os);

        OE_TEST(n == file_size / 8);
    }

    OE_TEST(truncate(path, 2) == 0);
    OE_TEST(stat(path, &st) == 0);
    OE_TEST(st.st_size == 2);

    OE_TEST(truncate(path, 0) == 0);
    OE_TEST(stat(path, &st) == 0);
    OE_TEST(st.st_size == 0);
}

static void _test_mkdir(const char* target)
{
    char path1[FS_PATH_MAX];
    char path2[FS_PATH_MAX];
    struct stat st;

    strlcpy(path1, target, sizeof(path1));
    strlcat(path1, "/mydir1", sizeof(path1));

    if (mkdir(path1, 0) != 0)
        OE_TEST(false);

    OE_TEST(stat(path1, &st) == 0);
    OE_TEST(st.st_mode & S_IFDIR);
    OE_TEST(!(st.st_mode & S_IFREG));

    strlcpy(path2, target, sizeof(path2));
    strlcat(path2, "/mydir1/mydir2", sizeof(path2));

    if (mkdir(path2, 0) != 0)
        OE_TEST(false);

    OE_TEST(stat(path2, &st) == 0);
    OE_TEST(st.st_mode & S_IFDIR);
    OE_TEST(!(st.st_mode & S_IFREG));

    OE_TEST(rmdir(path1) != 0);
    OE_TEST(rmdir(path2) == 0);
    OE_TEST(rmdir(path1) == 0);

    OE_TEST(stat(path1, &st) != 0);
    OE_TEST(stat(path2, &st) != 0);
}

static void _test_opendir(const char* target)
{
    char path[FS_PATH_MAX];
    char child_path[FS_PATH_MAX];
    struct stat st;
    DIR* dir;

    strlcpy(path, target, sizeof(path));
    strlcat(path, "/parent", sizeof(path));

    if (mkdir(path, 0) != 0)
        OE_TEST(false);

    OE_TEST(stat(path, &st) == 0);
    OE_TEST(st.st_mode & S_IFDIR);
    OE_TEST(!(st.st_mode & S_IFREG));

    strlcpy(child_path, path, sizeof(child_path));
    strlcat(child_path, "/child1", sizeof(child_path));
    OE_TEST(mkdir(child_path, 0) == 0);

    strlcpy(child_path, path, sizeof(child_path));
    strlcat(child_path, "/child2", sizeof(child_path));
    OE_TEST(mkdir(child_path, 0) == 0);

    strlcpy(child_path, path, sizeof(child_path));
    strlcat(child_path, "/child3", sizeof(child_path));
    OE_TEST(mkdir(child_path, 0) == 0);

    /* Enumerate the directory entries. */
    {
        OE_TEST((dir = opendir(path)) != NULL);

        int n = 0;
        {
            struct dirent* ent;

            while ((ent = readdir(dir)))
            {
                printf("d_name{%s}\n", ent->d_name);

                switch (n)
                {
                    case 0:
                        OE_TEST(strcmp(ent->d_name, "..") == 0);
                        break;
                    case 1:
                        OE_TEST(strcmp(ent->d_name, ".") == 0);
                        break;
                    case 2:
                        OE_TEST(strcmp(ent->d_name, "child1") == 0);
                        break;
                    case 3:
                        OE_TEST(strcmp(ent->d_name, "child2") == 0);
                        break;
                    case 4:
                        OE_TEST(strcmp(ent->d_name, "child3") == 0);
                        break;
                    default:
                        OE_TEST(false);
                }

                n++;
            }
        }

        OE_TEST(n == 5);
        OE_TEST(closedir(dir) == 0);
    }
}

static const char* _mkpath(char* buf, const char* target, const char* path)
{
    strlcpy(buf, target, FS_PATH_MAX);
    strlcat(buf, path, FS_PATH_MAX);
    return buf;
}

static void _test_cwd(const char* target)
{
    char cwd[PATH_MAX];
    char buf[PATH_MAX];

    OE_TEST(getcwd(cwd, sizeof(cwd)) != NULL);
    getcwd(cwd, sizeof(cwd));
    OE_TEST(strcmp(cwd, "/") == 0);

    char path[PATH_MAX];
    strlcpy(path, target, sizeof(path));
    strlcat(path, "/./././delete/../home/./delete/./..", sizeof(path));

    char expected[PATH_MAX];
    strlcpy(expected, target, sizeof(expected));
    strlcat(expected, "/home", sizeof(expected));

    printf("expected=%s\n", expected);

    OE_TEST(access(expected, F_OK) != 0);
    OE_TEST(mkdir(expected, 0) == 0);
    OE_TEST(access(expected, F_OK) == 0);

    char real[PATH_MAX];
    OE_TEST(realpath(path, real) != NULL);
    printf("real=%s\n", real);
    OE_TEST(strcmp(real, expected) == 0);

    OE_TEST(mkdir(_mkpath(buf, target, "/ddd1"), 0) == 0);
    OE_TEST(mkdir(_mkpath(buf, target, "/ddd1/ddd2"), 0) == 0);
    OE_TEST(mkdir(_mkpath(buf, target, "/ddd1/ddd2/ddd3"), 0) == 0);

    OE_TEST(chdir(_mkpath(buf, target, "/ddd1")) == 0);
    OE_TEST(chdir(_mkpath(buf, target, "/ddd1/ddd2")) == 0);
    OE_TEST(chdir(_mkpath(buf, target, "/ddd1/ddd2/ddd3")) == 0);

    getcwd(cwd, sizeof(cwd));
    OE_TEST(strcmp(cwd, _mkpath(buf, target, "/ddd1/ddd2/ddd3")) == 0);

    OE_TEST(chdir("/") == 0);
    getcwd(cwd, sizeof(cwd));
    OE_TEST(strcmp(cwd, "/") == 0);
}

static void _test_fcntl(const char* target)
{
    char path[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    const size_t N = 100;

    /* Create the file. */
    {
        int fd;

        fd = creat(_mkpath(path, target, "/create"), 0);
        OE_TEST(fd >= 0);

        for (size_t i = 0; i < N; i++)
        {
            size_t n = sizeof(alphabet) - 1;
            OE_TEST(write(fd, alphabet, n) == n);
        }

        OE_TEST(close(fd) == 0);
    }

    /* Read the file. */
    {
        int fd;
        char buf[sizeof(alphabet)];
        size_t m = 0;

        fd = open(_mkpath(path, target, "/create"), O_RDONLY, 0);
        OE_TEST(fd >= 0);

        for (size_t i = 0; i < N; i++)
        {
            size_t n = sizeof(alphabet) - 1;
            OE_TEST(read(fd, buf, sizeof(buf) - 1) == n);
            OE_TEST(memcmp(buf, alphabet, sizeof(buf) - 1) == 0);
            m++;
        }

        OE_TEST(m == N);
        OE_TEST(close(fd) == 0);
    }

    /* Remove the file. */
    OE_TEST(unlink(_mkpath(path, target, "/create")) == 0);
}

void run_tests(const char* target)
{
    fs_t* fs = fs_lookup(target, NULL);

    OE_TEST(fs != NULL);

    /* Create some files. */
    _create_files(fs);

    /* Create some directories. */
    _create_dirs(fs);

    /* Dump some directories. */
    _dump_dir(fs, "/");
    _dump_dir(fs, "/dir-0001");
    _dump_dir(fs, "/aaa/bbb/ccc");

    /* Test updating of a file. */
    {
        const char path[] = "/filename-0001";
        _update_file(fs, path);
        _dump_file(fs, path);
        _read_alphabet_file(target, path);
        _truncate_file(fs, path);
        _dump_file(fs, path);
    }

    /* Create "/aaa/bbb/ccc/myfile" */
    _dump_dir(fs, "/aaa/bbb/ccc");
    _create_myfile(fs);
    _dump_dir(fs, "/aaa/bbb/ccc");
    _dump_dir(fs, "/aaa/bbb/ccc");

    /* Lookup "/aaa/bbb/ccc/myfile". */
    {
        char suffix[FS_PATH_MAX];
        char path[FS_PATH_MAX];

        strlcpy(path, target, sizeof(path));
        strlcat(path, "/aaa/bbb/ccc/myfile", sizeof(path));

        fs_t* tmp = fs_lookup(path, suffix);
        OE_TEST(tmp == fs);
        OE_TEST(strcmp(suffix, "/aaa/bbb/ccc/myfile") == 0);
    }

    /* Try fopen() */
    {
        char path[FS_PATH_MAX];
        char buf[512];

        strlcpy(path, target, sizeof(path));
        strlcat(path, "/aaa/bbb/ccc/myfile", sizeof(path));

        printf("path=%s\n", path);
        FILE* is = fopen(path, "rb");
        OE_TEST(is != NULL);

        ssize_t n = fread(buf, 1, sizeof(buf), is);
        OE_TEST(n == 13);
        OE_TEST(strcmp(buf, "Hello World!") == 0);

        fclose(is);
    }

    _remove_file(fs, "/aaa/bbb/ccc/myfile");

    /* Remove some directories. */
    _dump_dir(fs, "/aaa/bbb");
    _remove_dir(fs, "/aaa/bbb/ccc");
    _dump_dir(fs, "/aaa/bbb");

    /* Remove some directories. */
    _dump_dir(fs, "/aaa");
    _remove_dir(fs, "/aaa/bbb");
    _dump_dir(fs, "/aaa");

    /* Remove some directories. */
    _dump_dir(fs, "/");
    _remove_dir(fs, "/aaa");
    _dump_dir(fs, "/");

    /* Remove directories named like dir-NNNN */
    _remove_dir_nnnn(fs);
    _dump_dir(fs, "/");

    /* Remove files named like filename-NNNN */
    _remove_files_nnnn(fs);
    _dump_dir(fs, "/");

    /* Test the lseek function. */
    _test_lseek(fs);

    /* Test the link function. */
    _test_links(fs);

    /* Test the rename function. */
    _test_rename(fs, target);

    /* Read posix read and write functions. */
    _write_alphabet_file(target, "/alphabet");
    _read_alphabet_file(target, "/alphabet");

    _test_truncate(target);

    _test_mkdir(target);
    _test_opendir(target);

    _test_cwd(target);

    _test_fcntl(target);
}

static void _test_hostfs()
{
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    struct stat buf;

    /* Mount the file system. */
    OE_TEST(fs_mount_hostfs("/mnt/hostfs") == 0);

    /* Remove the file if it exists. */
    unlink("/mnt/hostfs/tmp/myfile");
    unlink("/mnt/hostfs/tmp/myfile2");
    rmdir("/mnt/hostfs/tmp/mydir");

    /* Create a file with alphabet characters terminated by zero. */
    {
        FILE* os;

        OE_TEST((os = fopen("/mnt/hostfs/tmp/myfile", "wb")) != NULL);
        OE_TEST(fwrite(alphabet, 1, sizeof(alphabet), os) == sizeof(alphabet));
        OE_TEST(fclose(os) == 0);
    }

    /* Read the file and make sure the contents are the same. */
    {
        FILE* is;
        char buf[sizeof(alphabet)];

        OE_TEST((is = fopen("/mnt/hostfs/tmp/myfile", "rb")) != NULL);
        OE_TEST(fread(buf, 1, sizeof(buf), is) == sizeof(buf));
        OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);

        rewind(is);
        OE_TEST(ftell(is) == 0);

        OE_TEST(fseek(is, 7, SEEK_SET) == 0);
        OE_TEST(ftell(is) == 7);
        OE_TEST(fread(buf, 1, 4, is) == 4);
        OE_TEST(ftell(is) == 7 + 4);
        OE_TEST(memcmp(buf, "hijk", 4) == 0);

        OE_TEST(fclose(is) == 0);
    }

    /* Test stat() */
    OE_TEST(stat("/mnt/hostfs/tmp/myfile", &buf) == 0);
    OE_TEST(buf.st_size == sizeof(alphabet));
    OE_TEST(buf.st_nlink == 1);
    OE_TEST(S_ISREG(buf.st_mode));

    /* Test link() */
    OE_TEST(link("/mnt/hostfs/tmp/myfile", "/mnt/hostfs/tmp/myfile2") == 0);

    /* Test stat() */
    OE_TEST(stat("/mnt/hostfs/tmp/myfile2", &buf) == 0);
    OE_TEST(buf.st_size == sizeof(alphabet));
    OE_TEST(buf.st_nlink == 2);
    OE_TEST(S_ISREG(buf.st_mode));

    /* Scan the /tmp directory. */
    {
        DIR* dir;
        struct dirent* ent;
        size_t count = 0;

        OE_TEST((dir = opendir("/mnt/hostfs/tmp")) != NULL);

        while ((ent = readdir(dir)))
            count++;

        OE_TEST(count > 0);
        OE_TEST(closedir(dir) == 0);
    }

    OE_TEST(rename("/mnt/hostfs/tmp/myfile", "/mnt/hostfs/tmp/myfile3") == 0);
    OE_TEST(stat("/mnt/hostfs/tmp/myfile3", &buf) == 0);
    OE_TEST(buf.st_size == sizeof(alphabet));
    OE_TEST(buf.st_nlink == 2);
    OE_TEST(S_ISREG(buf.st_mode));

    OE_TEST(truncate("/mnt/hostfs/tmp/myfile3", 4) == 0);
    OE_TEST(stat("/mnt/hostfs/tmp/myfile3", &buf) == 0);
    OE_TEST(buf.st_size == 4);

    /* Remove the file if it exists. */
    OE_TEST(unlink("/mnt/hostfs/tmp/myfile3") == 0);
    OE_TEST(unlink("/mnt/hostfs/tmp/myfile2") == 0);

    OE_TEST(mkdir("/mnt/hostfs/tmp/mydir", 0) == 0);
    OE_TEST(stat("/mnt/hostfs/tmp/mydir", &buf) == 0);
    OE_TEST(S_ISDIR(buf.st_mode));

    OE_TEST(rmdir("/mnt/hostfs/tmp/mydir") == 0);

    /* Unmount the file system. */
    OE_TEST(fs_unmount("/mnt/hostfs") == 0);
}

int test_oefs(const char* oefs_filename)
{
    const uint32_t flags = FS_MOUNT_FLAG_MKFS;
    size_t num_blocks = 4 * 4096;
    int rc;
    uint8_t key[FS_MOUNT_KEY_SIZE] = {
        0x0f, 0xf0, 0x31, 0xe3, 0x93, 0xdf, 0x46, 0x7b, 0x9a, 0x33, 0xe8,
        0x3c, 0x55, 0x11, 0xac, 0x52, 0x9e, 0xd4, 0xb1, 0xad, 0x10, 0x16,
        0x4f, 0xd9, 0x92, 0x19, 0x93, 0xcc, 0xa9, 0x0e, 0xcb, 0xed,
    };

    /* Mount host file. */
    const char target1[] = "/mnt/oefs";
    rc = fs_mount_oefs(oefs_filename, target1, flags, num_blocks, key);
    OE_TEST(rc == 0);

    /* Mount enclave memory. */
    const char target2[] = "/mnt/ramfs";
    rc = fs_mount_oefs(NULL, target2, flags, num_blocks, NULL);
    OE_TEST(rc == 0);

    run_tests(target1);
    run_tests(target2);

    rc = fs_unmount(target1);
    rc = fs_unmount(target2);

    _test_hostfs();

    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,         /* ProductID */
    1,         /* SecurityVersion */
    true,      /* AllowDebug */
    10 * 1024, /* HeapPageCount */
    10 * 1024, /* StackPageCount */
    2);        /* TCSCount */
