# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (UNIX)
	add_subdirectory(enc)
endif()

add_test(
    tests/oefs 
    ./host/oefs_host 
    ./enc/oefs_enc 
    /tmp/oefs)
