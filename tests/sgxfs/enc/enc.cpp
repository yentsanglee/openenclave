#include <errno.h>
#include <errno.h>
#include <openenclave/internal/tests.h>
#include <openenclave/internal/fs.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../../sgxfs/common/sgxfs.h"
#include "../../../hostfs/common/hostfs.h"
#include "sgxfs_t.h"

#ifdef FILENAME_MAX
#undef FILENAME_MAX
#endif

#ifdef FOPEN_MAX
#undef FOPEN_MAX
#endif

#include "../../../3rdparty/linux-sgx/linux-sgx/common/inc/sgx_tprotected_fs.h"

static void _test1()
{
    const char path[] = "/tmp/pfs.test";

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
        else {
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

static void _test_fs(oe_fs_t* fs)
{
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];
    const size_t N = 16;
    FILE* stream;
    size_t m = 0;

    stream = oe_fopen(fs, "/tmp/sgxfs/myfile", "w", NULL);
    OE_TEST(stream != NULL);

    /* Write to the file */
    for (size_t i = 0; i < N; i++)
    {
        ssize_t n = oe_fwrite(alphabet, 1, sizeof(alphabet), stream);
        OE_TEST(n == sizeof(alphabet));
        m += n;
    }

    OE_TEST(m == sizeof(alphabet) * N);
    OE_TEST(oe_fflush(stream) == 0);
    OE_TEST(oe_fclose(stream) == 0);

    /* Reopen the file for read. */
    stream = oe_fopen(fs, "/tmp/sgxfs/myfile", "r", NULL);
    OE_TEST(stream != NULL);

    /* Read from the file. */
    for (size_t i = 0, m = 0; i < N; i++)
    {
        ssize_t n = oe_fread(buf, 1, sizeof(buf), stream);
        OE_TEST(n == sizeof(buf));
        OE_TEST(memcmp(buf, alphabet, sizeof(alphabet)) == 0);
        printf("buf{%s}\n", buf);
        m += n;
    }

    OE_TEST(m == sizeof(alphabet) * N);

    oe_fclose(stream);
}

void enc_test()
{
    _test1();
    _test_fs(&oe_sgxfs);
    _test_fs(&oe_hostfs);

    oe_release(&oe_hostfs);
    oe_release(&oe_sgxfs);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    2);   /* TCSCount */
