#define _GNU_SOURCE

// clang-format off
#include <openenclave/enclave.h>
extern oe_fs_t oe_sgxfs;
#define OE_DEFAULT_FS (&oe_sgxfs)
#include <stdio.h>
// clang-format on

#include <limits.h>
#include <string.h>
#include <openenclave/internal/tests.h>

void test_default_fs_macro(const char* tmp_dir)
{
    FILE* stream;
    char path[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];

    oe_fs_set_default(NULL);

    strlcpy(path, tmp_dir, sizeof(path));
    strlcat(path, "/test_default_fs_macro.file", sizeof(path));

    /* Write the alphabet to the file. */
    OE_TEST((stream = oe_fopen(OE_DEFAULT_FS, path, "wb")));
    OE_TEST(fwrite(alphabet, 1, sizeof(alphabet), stream) == sizeof(alphabet));
    OE_TEST(fclose(stream) == 0);

    /* Read the alphabet back from the file. */
    OE_TEST((stream = oe_fopen(OE_DEFAULT_FS, path, "rb")));
    OE_TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    OE_TEST(fclose(stream) == 0);

    /* Read the alphabet back using the defaulted standard APIs. */
    OE_TEST((stream = fopen(path, "rb")));
    OE_TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    OE_TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    OE_TEST(fclose(stream) == 0);

    OE_TEST(oe_remove(OE_DEFAULT_FS, path) == 0);
}
