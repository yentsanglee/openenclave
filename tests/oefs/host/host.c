// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "oefs_u.h"
#include "../../../fs/cpio.h"

static void _create_cpio_archive()
{
    fs_cpio_t* cpio;
    const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    fs_cpio_entry_t entry;

    cpio = fs_cpio_open("/tmp/my.cpio", FS_CPIO_FLAG_CREATE);
    OE_TEST(cpio != NULL);

    memset(&entry, 0, sizeof(entry));
    entry.size = sizeof(alphabet);
    entry.mode = FS_CPIO_MODE_IFREG;
    strcpy(entry.name, "alphabet");

    OE_TEST(fs_cpio_write_entry(cpio, &entry) == 0);

    OE_TEST(fs_cpio_write_data(cpio, alphabet, sizeof(alphabet)) == 0);

    OE_TEST(fs_cpio_close(cpio) == 0);
}

int main(int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enclave;
    oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    const uint32_t flags = oe_get_create_flags();
    int ret;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH OEFS_PATH\n", argv[0]);
        return 1;
    }

    r = oe_create_oefs_enclave(argv[1], type, flags, NULL, 0, &enclave);
    if (r != OE_OK)
    {
        fprintf(stderr, "%s: oe_create_oefs_enclave() failed\n", argv[0]);
        exit(1);
    }

    /* Truncate or create the OEFS file. */
    {
        FILE* os;
        OE_TEST((os = fopen(argv[2], "wb")) != NULL);
        fclose(os);
    }

    /* Create the CPIO archive. */
    _create_cpio_archive();

    r = test_oefs(enclave, &ret, argv[2]);
    if (r != OE_OK || ret != 0)
    {
        fprintf(stderr, "%s: test_oefs() failed\n", argv[0]);
        exit(1);
    }

#if 1
    r = oe_terminate_enclave(enclave);
    if (r != OE_OK)
    {
        fprintf(stderr, "%s: oe_terminate_enclave() failed\n", argv[0]);
        exit(1);
    }
#endif

    printf("=== passed all tests (oefs)\n");

    return 0;
}
