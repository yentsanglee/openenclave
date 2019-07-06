// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fcntl.h>
#include <limits.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/entropy.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/print.h>
#include <openenclave/internal/tests.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/time.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
#include "vsample1_t.h"

uint64_t test_ecall(uint64_t x)
{
    uint64_t retval;
    static oe_spinlock_t _spin = OE_SPINLOCK_INITIALIZER;

    oe_spin_lock(&_spin);
    oe_sleep_msec(10);
    oe_spin_unlock(&_spin);

#if 0
    {
        static oe_mutex_t _mutex = OE_MUTEX_INITIALIZER;
        uint8_t buf[32];
        char str[sizeof(buf) * 2 + 1];
        oe_mutex_lock(&_mutex);
        oe_get_entropy(buf, sizeof(buf));
        oe_hex_string(str, sizeof(str), buf, sizeof(buf));
        oe_host_printf("tmp=%s\n", str);
        oe_mutex_unlock(&_mutex);
    }
#endif

    if (test_ocall(&retval, x) != OE_OK || retval != x)
    {
        oe_host_printf("test_ocall() failed\n");
        oe_abort();
    }

    // OE_TEST(false);

    return retval;
}

int test_time(void)
{
    extern time_t oe_time(time_t * tloc);

    for (size_t i = 0; i < 10; i++)
    {
        oe_host_printf("sleep...\n");
        oe_sleep_msec(1000);

        oe_host_printf("time.secs=%lu\n", oe_time(NULL));
        oe_host_printf("time.msec=%lu\n", oe_get_time());
    }

    return 0;
}

void test_files1(const char* path)
{
    char filename[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";

    OE_TEST(oe_load_module_host_file_system() == OE_OK);

    OE_TEST(mount("/", "/", OE_HOST_FILE_SYSTEM, 0, NULL) == 0);

    strlcpy(filename, path, sizeof(filename));
    strlcat(filename, "/tmpfile", sizeof(filename));

    /* Create a file that contains the alphabet. */
    {
        FILE* os;

        OE_TEST((os = fopen(filename, "w")) != NULL);
        OE_TEST(fwrite(alphabet, 1, sizeof(alphabet), os) == sizeof(alphabet));
        OE_TEST(fclose(os) == 0);
    }

    /* Create a file that contains the alphabet. */
    {
        FILE* is;
        char buf[sizeof(alphabet)];

        OE_TEST((is = fopen(filename, "r")) != NULL);
        OE_TEST(fread(buf, 1, sizeof(buf), is) == sizeof(buf));
        OE_TEST(fclose(is) == 0);
        OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    }

    OE_TEST(umount("/") == 0);
}

void test_files2(const char* path)
{
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char filename[PATH_MAX];

    /* Suppress Valgrind error. */
    memset(filename, 0, sizeof(filename));

    OE_TEST(oe_load_module_host_file_system() == OE_OK);

    OE_TEST(mount("/", "/", OE_HOST_FILE_SYSTEM, 0, NULL) == 0);

    strlcpy(filename, path, sizeof(filename));
    strlcat(filename, "/tmpfile", sizeof(filename));

    /* Create a file that contains the alphabet. */
    {
        int fd;

        OE_TEST((fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY)) >= 0);
        OE_TEST(write(fd, alphabet, sizeof(alphabet)) == sizeof(alphabet));
        OE_TEST(close(fd) == 0);
    }

    /* Create a file that contains the alphabet. */
    {
        int fd;
        char buf[sizeof(alphabet)];

        OE_TEST((fd = open(filename, O_RDONLY)) >= 0);
        OE_TEST(read(fd, buf, sizeof(buf)) == sizeof(buf));
        OE_TEST(close(fd) == 0);
        OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    }

    OE_TEST(umount("/") == 0);
}

void test_files(const char* path)
{
    test_files1(path);
    test_files2(path);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* AllowDebug */
    1024, /* HeapPageCount */
    1024, /* StackPageCount */
    8);   /* TCSCount */
