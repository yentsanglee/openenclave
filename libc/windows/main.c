#include <stdlib.h>
#include "oelibc.h"
#include "tests.h"

void oemain(void)
{
    test_string();
    test_stdarg();

    print("==== passed all tests\n");
}
