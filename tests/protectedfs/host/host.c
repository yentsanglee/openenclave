#include <openenclave/host.h>
#include "protectedfs_u.h"

void host_test()
{
}

int main(int argc, const char* argv[])
{
    oe_result_t result;
    int ret = 1;
    oe_enclave_t* enclave = NULL; // Create the enclave
    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

    /* Install the protected file system. */
    {
        extern void oe_install_protectedfs(void);
        oe_install_protectedfs();
    }

    printf("Starting enclave with %s\n", argv[1]);
    result = oe_create_protectedfs_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "oe_create_protectedfs_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
        goto exit;
    }

    result = enc_test(enclave);
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "test_enc(): result=%u (%s)\n",
            result,
            oe_result_str(result));
        goto exit;
    }

    ret = 0;

exit:
    // Clean up the enclave if we created one
    if (enclave)
        oe_terminate_enclave(enclave);

    return ret;
}
