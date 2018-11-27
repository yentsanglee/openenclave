#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include "fs_u.h"

void host_test()
{
}

int main(int argc, const char* argv[])
{
    oe_result_t result;
    int ret = 1;
    oe_enclave_t* enclave = NULL; // Create the enclave
    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

    /* Check command-line arguments. */
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s enclave source-dir binary-dir\n", argv[0]);
        exit(1);
    }

    /* Install hostfs */
    {
        extern void oe_install_hostfs(void);
        oe_install_hostfs();
    }

    /* Install sgxfs */
    {
        extern void oe_install_sgxfs(void);
        oe_install_sgxfs();
    }

    /* Install oefs */
    {
        extern void oe_install_oefs(void);
        oe_install_oefs();
    }

    printf("Starting enclave with %s\n", argv[1]);
    result = oe_create_fs_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        fprintf(
            stderr,
            "oe_create_sgxfs_enclave(): result=%u (%s)\n",
            result,
            oe_result_str(result));
        goto exit;
    }

    result = enc_test(enclave, argv[2], argv[3]);
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
        OE_TEST(oe_terminate_enclave(enclave) == 0);

    return ret;
}
