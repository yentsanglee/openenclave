// clang-format off
#define OE_DEFAULT_FS (&oe_sgxfs)
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
// clang-format on

/* Cannot use OE_TEST since that would introduce OE headers. */
#define TEST(COND)                              \
    do                                          \
    {                                           \
        if (!(COND))                            \
        {                                       \
            fprintf(                            \
                stderr,                         \
                "Test failed: %s(%u): %s %s\n", \
                __FILE__,                       \
                __LINE__,                       \
                __FUNCTION__,                   \
                #COND);                         \
            abort();                            \
        }                                       \
    } while (0)

void test_default_fs_macro(const char* tmp_dir)
{
    FILE* stream;
    char path[PATH_MAX];
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    char buf[sizeof(alphabet)];

    //oe_fs_set_default(NULL);

    strlcpy(path, tmp_dir, sizeof(path));
    strlcat(path, "/test_default_fs_macro.file", sizeof(path));

    /* Write the alphabet to the file. */
    TEST((stream = oe_fopen(OE_DEFAULT_FS, path, "wb")));
    TEST(fwrite(alphabet, 1, sizeof(alphabet), stream) == sizeof(alphabet));
    TEST(fclose(stream) == 0);

    /* Read the alphabet back from the file. */
    TEST((stream = oe_fopen(OE_DEFAULT_FS, path, "rb")));
    TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    TEST(fclose(stream) == 0);

    /* Read the alphabet back using the defaulted standard APIs. */
    TEST((stream = fopen(path, "rb")));
    TEST(fread(buf, 1, sizeof(buf), stream) == sizeof(buf));
    TEST(memcmp(buf, alphabet, sizeof(buf)) == 0);
    TEST(fclose(stream) == 0);

    TEST(oe_remove(OE_DEFAULT_FS, path) == 0);
}
