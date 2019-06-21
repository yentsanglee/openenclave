# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../echod.edl enclave gen)

add_executable(echod_enc enc.c ${gen})

target_link_libraries(echod_enc oelibc oehostsock oeenclave)
