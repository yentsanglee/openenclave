# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(enc)
add_subdirectory(host)
add_test(tests/curl_openssl_client host/curl_openssl_clienthost ./enc/curl_openssl_clientenc)
