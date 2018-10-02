# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(add_enclave_executable)
add_executable(libcxx2_enc 
    enc.cpp
    fuzzing.cpp
    memory_resource.cpp)

target_compile_options(libcxx2_enc PRIVATE
    -fsized-deallocation
    -fno-strict-return
    -Wno-unused-variable
    -Wno-return-type
    -Wno-macro-redefined
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)


target_include_directories(libcxx2_enc PRIVATE
    ..
    ${PROJECT_SOURCE_DIR}/3rdparty/libcxx/libcxx/test
    ${PROJECT_SOURCE_DIR}/3rdparty/libcxx/libcxx/test/support
    ${PROJECT_SOURCE_DIR}/3rdparty/libcxx/libcxx/fuzzing)


target_link_libraries(libcxx2_enc oelibcxx oeenclave)
