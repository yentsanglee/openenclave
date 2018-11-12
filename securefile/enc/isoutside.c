#include <openenclave/enclave.h>

int sgx_is_outside_enclave(const void* addr, size_t size)
{
    return oe_is_outside_enclave(addr, size);
}
