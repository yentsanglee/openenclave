#include "tests.h"
#include <string.h>

void _test_strcpy(void)
{
    char buf[1024];
    strcpy(buf, "hello");
    TEST(strcmp(buf, "hello") == 0);
}

void _test_strcmp(void)
{
    TEST(strcmp("aaa", "aaa") == 0);
}

void _test_memcpy(void)
{
    char buf[1024];
    memcpy(buf, "hello", 5);
    TEST(memcmp(buf, "hello", 5) == 0);
}

void _test_memcmp(void)
{
    TEST(memcmp("aaa", "aaa", 3) == 0);
}

void test_string(void)
{
    _test_strcpy();
    _test_strcmp();
    _test_memcpy();
    _test_memcmp();
}
