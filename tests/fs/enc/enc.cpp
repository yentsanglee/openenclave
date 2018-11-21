#include <errno.h>
#include <errno.h>
#include <limits.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/hostfs.h>
#include <openenclave/internal/muxfs.h>
#include <openenclave/internal/sgxfs.h>
#include <openenclave/internal/tests.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../../fs/cpio/cpio.h"
#include "fs_t.h"
#include "iot.h"

#ifdef FILENAME_MAX
#undef FILENAME_MAX
#endif

#ifdef FOPEN_MAX
#undef FOPEN_MAX
#endif

#include "../../../3rdparty/linux-sgx/linux-sgx/common/inc/sgx_tprotected_fs.h"

oe_fs_t oe_default_fs = oe_hostfs;

static const char* _mkpath(
    char buf[PATH_MAX],
    const char* target,
    const char* path)
{
    strlcpy(buf, target, PATH_MAX);
    strlcat(buf, path, PATH_MAX);
    return buf;
}

static void _test_default_fs(const char* tmp_dir)
{
    char path[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];
    FILE* os;

    _mkpath(path, tmp_dir, "/default_fs.test");

    OE_TEST((os = fopen(path, "wb")) != NULL);
    OE_TEST(fwrite(alphabet, 1, sizeof(alphabet), os) == sizeof(alphabet));
    OE_TEST(fclose(os) == 0);

    OE_TEST((os = fopen(path, "rb")) != NULL);
    OE_TEST(fread(buf, 1, sizeof(alphabet), os) == sizeof(alphabet));
    OE_TEST(fclose(os) == 0);

    OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);

    OE_TEST(remove(path) == 0);
}

static void _test1(const char* tmp_dir)
{
    char path[PATH_MAX];
    _mkpath(path, tmp_dir, "/test1");

    printf("About to open a file\n");
    SGX_FILE* file = sgx_fopen_auto_key(path, "w");
    if (file != NULL)
    {
        printf("Successfully opened for write\n");

        char writedata[] = "hello world!";
        int bytes_written = sgx_fwrite(writedata, 1, sizeof(writedata), file);
        if (bytes_written == sizeof(writedata))
        {
            printf("Successfully wrote data\n");
        }
        else
        {
            printf("Failed to write data. sgx_ferror=%d\n", sgx_ferror(file));
            OE_TEST(false);
        }

        sgx_fclose(file);
        printf("Closed file\n");

        file = sgx_fopen_auto_key(path, "r");
        if (file != NULL)
        {
            printf("Successfully opened for read\n");

            char readdata[1000];
            int bytes_read = sgx_fread(readdata, 1, sizeof(readdata), file);
            if (bytes_read == bytes_written)
            {
                printf("Successfully read data\n");
                if (strcmp(writedata, readdata) == 0)
                {
                    printf("Read and write data were the same\n");
                }
                else
                {
                    printf("Read and write data were different\n");
                }
            }
            else
            {
                printf(
                    "Failed to read data. sgx_ferror=%d\n", sgx_ferror(file));
                OE_TEST(false);
            }

            sgx_fclose(file);
            printf("Closed file\n");
        }
        else
        {
            printf("Failed to open file for read\n");
            OE_TEST(false);
        }
    }
    else
    {
        printf(
            "Failed to open file. errno=%d, \"%s\"\n", errno, strerror(errno));
        OE_TEST(false);
    }
}

static void _test2(oe_fs_t* fs, const char* tmp_dir)
{
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];
    const size_t N = 1600;
    FILE* stream;
    size_t m = 0;
    char path[PATH_MAX];

    _mkpath(path, tmp_dir, "/test2");

    stream = oe_fopen(fs, path, "w");
    OE_TEST(stream != NULL);

    /* Write to the file */
    for (size_t i = 0; i < N; i++)
    {
        ssize_t n = fwrite(alphabet, 1, sizeof(alphabet), stream);
        OE_TEST(n == sizeof(alphabet));
        m += n;
    }

    OE_TEST(m == sizeof(alphabet) * N);
    OE_TEST(fflush(stream) == 0);
    OE_TEST(fclose(stream) == 0);

    /* Reopen the file for read. */
    stream = oe_fopen(fs, path, "r");
    OE_TEST(stream != NULL);

    /* Read from the file. */
    for (size_t i = 0, m = 0; i < N; i++)
    {
        ssize_t n = fread(buf, 1, sizeof(buf), stream);
        OE_TEST(n == sizeof(buf));
        OE_TEST(memcmp(buf, alphabet, sizeof(alphabet)) == 0);
        // printf("buf{%s}\n", buf);
        m += n;
    }

    OE_TEST(m == sizeof(alphabet) * N);
    fclose(stream);
}

static void _test3(oe_fs_t* fs, const char* tmp_dir)
{
    DIR* dir;
    struct dirent* entry;
    size_t m = 0;

    OE_TEST((dir = oe_opendir(fs, tmp_dir)) != NULL);

    while ((entry = readdir(dir)))
    {
        m++;

        if (strcmp(entry->d_name, ".") == 0)
            continue;

        if (strcmp(entry->d_name, "..") == 0)
            continue;

        if (strcmp(entry->d_name, "test1") == 0)
            continue;

        if (strcmp(entry->d_name, "test2") == 0)
            continue;

        if (strcmp(entry->d_name, "cpio.file") == 0)
            continue;

        if (strcmp(entry->d_name, "cpio.dir") == 0)
            continue;

        if (strcmp(entry->d_name, "test_sgxfs_with_key") == 0)
            continue;

        printf("ERROR: found file: %s\n", entry->d_name);
        OE_TEST(false);
    }

    OE_TEST(m >= 4);

    closedir(dir);
}

static void _test4(oe_fs_t* fs, const char* src_dir, const char* tmp_dir)
{
    char tests_dir[PATH_MAX];
    char cpio_file[PATH_MAX];
    char cpio_dir[PATH_MAX];

    _mkpath(tests_dir, src_dir, "/tests");
    _mkpath(cpio_file, tmp_dir, "/cpio.file");
    _mkpath(cpio_dir, tmp_dir, "/cpio.dir");

    oe_fs_set_default(fs);
    OE_TEST(oe_cpio_pack(tests_dir, cpio_file) == 0);
    mkdir(cpio_dir, 0777);
    OE_TEST(oe_cpio_unpack(cpio_file, cpio_dir) == 0);
    oe_fs_set_default(NULL);
}

static void _test_sgxfs_with_key(const char* tmp_dir)
{
    FILE* stream;
    char path[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];
    uint8_t key[16];
    uint8_t wrong_key[16];

    OE_TEST(oe_random(key, sizeof(key)) == OE_OK);
    OE_TEST(oe_random(wrong_key, sizeof(wrong_key)) == OE_OK);
    OE_TEST(memcmp(key, wrong_key, sizeof(wrong_key)) != 0);

    strlcpy(path, tmp_dir, sizeof(path));
    strlcat(path, "/test_sgxfs_with_key", sizeof(path));

    /* Write the alphabet to the file. */
    OE_TEST((stream = oe_fopen(&oe_sgxfs, path, "wbk", key)));
    OE_TEST(fwrite(alphabet, 1, sizeof(alphabet), stream) == sizeof(alphabet));
    OE_TEST(fclose(stream) == 0);

    /* Make sure that opening the file with the wrong key fails. */
    OE_TEST((stream = oe_fopen(&oe_sgxfs, path, "rbk", wrong_key)) == NULL);

    /* Read the alphabet back from the file. */
    OE_TEST((stream = oe_fopen(&oe_sgxfs, path, "rbk", key)));
    OE_TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    OE_TEST(fclose(stream) == 0);

    OE_TEST(oe_remove(&oe_sgxfs, path) == 0);
}

void enc_test(const char* src_dir, const char* bin_dir)
{
    static char tmp_dir[PATH_MAX];
    struct stat buf;

    /* Create the temporary directory (if it does not already exist). */
    {
        _mkpath(tmp_dir, bin_dir, "/tests/fs/tmp");

        if (oe_stat(&oe_hostfs, tmp_dir, &buf) != 0)
            OE_TEST(oe_mkdir(&oe_hostfs, tmp_dir, 0777) == 0);
    }

    _test_default_fs(tmp_dir);
    _test1(tmp_dir);
    _test2(&oe_sgxfs, tmp_dir);
    _test2(&oe_hostfs, tmp_dir);
    _test3(&oe_hostfs, tmp_dir);
    _test3(&oe_sgxfs, tmp_dir);
    _test4(&oe_hostfs, src_dir, tmp_dir);

    /* Test the multiplexer: hostfs -> hostfs */
    {
        char mux_src_dir[PATH_MAX];
        char mux_tmp_dir[PATH_MAX];
        _mkpath(mux_src_dir, "/hostfs", src_dir);
        _mkpath(mux_tmp_dir, "/hostfs", tmp_dir);
        _test4(&oe_muxfs, mux_src_dir, mux_tmp_dir);
    }

    /* Test the multiplexer: hostfs -> sgxfs */
    {
        char mux_src_dir[PATH_MAX];
        char mux_tmp_dir[PATH_MAX];
        _mkpath(mux_src_dir, "/hostfs", src_dir);
        _mkpath(mux_tmp_dir, "/sgxfs", tmp_dir);
        _test4(&oe_muxfs, mux_src_dir, mux_tmp_dir);
    }

    /* Test the IOT stdio.h definitions. */
    test_iot_scenario(tmp_dir);

    _test_sgxfs_with_key(tmp_dir);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
