# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (OE_SGX)
add_subdirectory(sgx_secure)
elseif(OE_TRUSTZONE)
add_subdirectory(optee_secure)
endif()

add_subdirectory(socket_host)
