// clang-format off
#define _GNU_SOURCE
#define OE_SECURE_POSIX_FILE_API
#define OE_USE_FILE_DEFAULT
#include <limits.h>
#include <string.h>
#include <openenclave/enclave.h>
#include <openenclave/iot/stdio.h>
#include <openenclave/internal/tests.h>
// clang-format on

void test_iot_scenario(const char* tmp_dir)
{
    FILE* stream;
    char path[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];

    strlcpy(path, tmp_dir, sizeof(path));
    strlcat(path, "/iot.file", sizeof(path));

    /* Write the alphabet to the file. */
    OE_TEST((stream = oe_fopen(OE_FILE_SECURE_BEST_EFFORT, path, "wb")));
    OE_TEST(fwrite(alphabet, 1, sizeof(alphabet), stream) == sizeof(alphabet));
    OE_TEST(fclose(stream) == 0);

    /* Read the alphabet back from the file. */
    OE_TEST((stream = oe_fopen(OE_FILE_SECURE_BEST_EFFORT, path, "rb")));
    OE_TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    OE_TEST(fclose(stream) == 0);

    /* Read the alphabet back using the defaulted standard APIs. */
    OE_TEST((stream = fopen(path, "rb", NULL)));
    OE_TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    OE_TEST(fclose(stream) == 0);

    OE_TEST(oe_remove(OE_FILE_SECURE_BEST_EFFORT, path) == 0);
}
