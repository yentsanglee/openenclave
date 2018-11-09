// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <limits.h>
#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <string.h>
#include "../../../fs/cpio.h"
#include "../../../fs/strings.h"
#include "oefs_u.h"

static void _create_cpio_archive(const char* src_dir, const char* bin_dir)
{
    char source[PATH_MAX];
    char target[PATH_MAX];

    fs_strlcpy(source, src_dir, sizeof(source));
    fs_strlcat(source, "/tests", sizeof(source));

    fs_strlcpy(target, bin_dir, sizeof(target));
    fs_strlcat(target, "/tests/oefs/host.tests.cpio", sizeof(target));

    OE_TEST(fs_cpio_pack(source, target) == 0);
}

static void _truncate_oefs_file(const char* bin_dir)
{
    char path[PATH_MAX];
    FILE* os;

    fs_strlcpy(path, bin_dir, sizeof(path));
    fs_strlcat(path, "/tests/oefs/tests.oefs", sizeof(path));

    OE_TEST((os = fopen(path, "wb")) != NULL);
    fclose(os);
}

int main(int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enclave;
    oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    const uint32_t flags = oe_get_create_flags();
    int ret;
    const char* src_dir;
    const char* bin_dir;

    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s ENCLAVE_PATH SRC_DIR BIN_DIR\n", argv[0]);
        return 1;
    }

    src_dir = argv[2];
    bin_dir = argv[3];

    /* Create the CPIO archive. */
    _create_cpio_archive(src_dir, bin_dir);

    /* Truncate or create the OEFS file. */
    _truncate_oefs_file(bin_dir);

    r = oe_create_oefs_enclave(argv[1], type, flags, NULL, 0, &enclave);
    if (r != OE_OK)
    {
        fprintf(stderr, "%s: oe_create_oefs_enclave() failed\n", argv[0]);
        exit(1);
    }

    r = test_oefs(enclave, &ret, src_dir, bin_dir);
    if (r != OE_OK || ret != 0)
    {
        fprintf(stderr, "%s: test_oefs() failed\n", argv[0]);
        exit(1);
    }

    r = oe_terminate_enclave(enclave);
    if (r != OE_OK)
    {
        fprintf(stderr, "%s: oe_terminate_enclave() failed\n", argv[0]);
        exit(1);
    }

    printf("=== passed all tests (oefs)\n");

    return 0;
}
