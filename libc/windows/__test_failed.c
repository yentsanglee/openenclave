#include <stdlib.h>
#include "tests.h"
#include "oelibc.h"

void __test_failed(const char* cond, const char* file, size_t line, const char* func)
{
    print("TEST failed: ");
    print(cond);
    print(" ");
    print(file);
    print(" ");
    print(func);
    print("\n");
    exit(1);
}
