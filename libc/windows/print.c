#include <string.h>
#include "oelibc.h"

void print(const char* str)
{
    size_t len = strlen(str);
    write_stdout(str, len);
}
