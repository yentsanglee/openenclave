#include <stdlib.h>
#include "oelibc.h"
#include "tests.h"

void oemain(void)
{
    test_string();
    test_stdarg();
    test_pow();

    print("==== passed all tests\n");
}
