# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(CMAKE_C_COMPILER clang-7)

set(OPEN_ENCLAVE_INCLUDE /opt/openenclave/include)

set(OPENSSL_EXTRA_INCLUDES /root/openssl_extras)

add_compile_options(
    -nostdinc
    -m64
    -fPIE
    -fno-stack-protector
    -fvisibility=hidden
    -ftls-model=local-exec
    -mllvm
    -x86-speculative-load-hardening)

include_directories(
    ${OPENSSL_EXTRA_INCLUDES}
    ${OPEN_ENCLAVE_INCLUDE}/openenclave/3rdparty/libc
    ${OPEN_ENCLAVE_INCLUDE}/openenclave/3rdparty
    ${OPEN_ENCLAVE_INCLUDE})
