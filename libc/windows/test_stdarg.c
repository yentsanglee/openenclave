#include <stdarg.h>
#include <string.h>
#include "tests.h"

static void _func(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    int a = va_arg(ap, int);
    double b = va_arg(ap, double);
    const char* c = va_arg(ap, const char*);
    char d = va_arg(ap, char);

    TEST(a == 1);
    TEST(b == 0.0);
    TEST(strcmp(c, "hello") == 0);
    TEST(d == 'c');

    va_end(ap);
}

void test_stdarg(void)
{
    _func("hello", 1, 0.0, "hello", 'c');

    print("==== passed test_stdarg()\n");
}
