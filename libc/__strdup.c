#include <openenclave/enclave.h>
#include <string.h>

char * __strdup(const char * string)
{
    return strdup(string);
}
