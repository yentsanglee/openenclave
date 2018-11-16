# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)


if (UNIX)
add_enclave_test(tests/ecall ./host ecall_host ./enc ecall_enc)
endif()

