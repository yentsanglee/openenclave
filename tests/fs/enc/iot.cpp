// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#define _FILE_DEFINED
#define OE_SECURE_POSIX_FILE_API
#include <openenclave/enclave.h>
#include <openenclave/iot/stdio.h>
#include <openenclave/internal/tests.h>
#include <openenclave/internal/enclavelibc.h>
// clang-format on

void test_iot(const char* tmp_dir)
{
    char path[128];
    static const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyz";

    oe_strlcpy(path, tmp_dir, sizeof(path));
    oe_strlcat(path, "/test_iot", sizeof(path));

    /* Create a file containing the alphabet. */
    {
        FILE* stream = fopen(path, "w");
        OE_TEST(stream);

        size_t n = fwrite(ALPHABET, 1, sizeof(ALPHABET), stream);
        OE_TEST(n == sizeof(ALPHABET));

        OE_TEST(fclose(stream) == 0);
    }

    /* Read the file containing the alphabet. */
    {
        FILE* stream = fopen(path, "r");
        OE_TEST(stream);
        char buf[sizeof(ALPHABET)];

        size_t n = fread(buf, 1, sizeof(ALPHABET), stream);
        OE_TEST(n == sizeof(ALPHABET));
        OE_TEST(oe_memcmp(buf, ALPHABET, sizeof(ALPHABET)) == 0);

        OE_TEST(fclose(stream) == 0);
    }
}
