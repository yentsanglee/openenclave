#include <openenclave/enclave.h>
#include <stdarg.h>
#include <string.h>

char * __strcat_chk(char * dest, const char * src, size_t destlen)
{
    return strcat(dest, src);
}
