#include <openenclave/bits/safecrt.h>
#include <sttdef.h>

int memset_s(void *s, size_t smax, int c, size_t n)
{
    return oe_memset_s(dst, smax, c, n);
}
