# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (UNIX)
	add_subdirectory(enc)
endif()

add_test(
    tests/ext2 
    ./host/ext2_host 
    ./enc/ext2_enc 
    /root/openenclave/libc/filesys/myext2)
