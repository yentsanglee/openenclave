# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_custom_target(oe_tssl_crypto 
    ${PROJECT_SOURCE_DIR}/3rdparty/openssl/Linux/build_openssl.sh)
