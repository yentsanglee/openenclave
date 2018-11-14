# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

add_enclave_test(tests/sgxfs 
    ./host sgxfs_host ./enc sgxfs_enc)
