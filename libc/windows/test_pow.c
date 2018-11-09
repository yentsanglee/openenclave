#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "tests.h"


void test_pow(void)
{
    double rslt = pow(7.0, 2.0);

    if (rslt == 49.0)  {
        print("==== passed test_pow()\n");
    }
    else {
        print("==== failed test_pow() rslt = %ld, should be 49.0\n", rslt);
    }
}
